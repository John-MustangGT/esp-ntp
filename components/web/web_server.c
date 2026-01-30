#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "nvs_storage.h"
#include <string.h>
#include <ctype.h>

static const char *TAG = "WEB_SERVER";
static httpd_handle_t server = NULL;

// HTML configuration page (served in AP mode)
static const char* config_html = 
    "<!DOCTYPE html><html><head><title>ESP-NTP Configuration</title>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>body{font-family:Arial;margin:20px;background:#f0f0f0;}"
    "form{background:white;padding:20px;border-radius:5px;max-width:400px;}"
    "input{width:100%;padding:10px;margin:5px 0;box-sizing:border-box;}"
    "button{background:#4CAF50;color:white;padding:10px 20px;border:none;cursor:pointer;width:100%;}"
    "button:hover{background:#45a049;}</style></head><body>"
    "<h1>ESP-NTP Configuration</h1>"
    "<form action='/save' method='POST'>"
    "<label>WiFi SSID:</label><input type='text' name='ssid' required><br>"
    "<label>WiFi Password:</label><input type='password' name='password' required><br>"
    "<label>NTP Server:</label><input type='text' name='ntp_server' value='pool.ntp.org'><br>"
    "<label>Timezone Offset (seconds):</label><input type='number' name='timezone' value='0'><br>"
    "<button type='submit'>Save & Reboot</button>"
    "</form></body></html>";

static const char* success_html = 
    "<!DOCTYPE html><html><head><title>Configuration Saved</title></head><body>"
    "<h1>Configuration Saved Successfully!</h1>"
    "<p>Device will reboot in 3 seconds...</p>"
    "</body></html>";

// Handler for root page (GET /)
static esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, config_html, strlen(config_html));
    return ESP_OK;
}

// URL decode helper
static void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) 
            && (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}

// Parse form data from POST body
static bool parse_form_data(const char *data, nvs_config_t *config) {
    char *saveptr;
    char *data_copy = strdup(data);
    if (!data_copy) return false;

    char *token = strtok_r(data_copy, "&", &saveptr);
    while (token != NULL) {
        char *eq = strchr(token, '=');
        if (eq) {
            *eq = '\0';
            char *key = token;
            char *value = eq + 1;
            
            char decoded_value[256];
            url_decode(decoded_value, value);

            if (strcmp(key, "ssid") == 0) {
                strncpy(config->ssid, decoded_value, sizeof(config->ssid) - 1);
            } else if (strcmp(key, "password") == 0) {
                strncpy(config->password, decoded_value, sizeof(config->password) - 1);
            } else if (strcmp(key, "ntp_server") == 0) {
                strncpy(config->ntp_server, decoded_value, sizeof(config->ntp_server) - 1);
            } else if (strcmp(key, "timezone") == 0) {
                config->timezone_offset_sec = atoi(decoded_value);
            }
        }
        token = strtok_r(NULL, "&", &saveptr);
    }

    free(data_copy);
    return true;
}

// Handler for config save (POST /save)
static esp_err_t save_handler(httpd_req_t *req) {
    char buf[512];
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf)) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // Parse and save configuration
    nvs_config_t config = {0};
    config.version = NVS_CONFIG_VERSION;
    
    if (!parse_form_data(buf, &config)) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Write to NVS
    esp_err_t err = nvs_config_write(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write config to NVS");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Send success response
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, success_html, strlen(success_html));

    ESP_LOGI(TAG, "Configuration saved, scheduling reboot...");
    
    // Schedule reboot after response sent
    extern void trigger_reboot(void);
    trigger_reboot();

    return ESP_OK;
}

// Handler for status API (GET /api/status)
static esp_err_t status_handler(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    
    // Add status information
    cJSON_AddStringToObject(root, "version", "1.0.0");
    cJSON_AddStringToObject(root, "device", "ESP-NTP");
    cJSON_AddBoolToObject(root, "config_valid", nvs_config_is_valid());
    
    const char *json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free((void *)json_str);
    cJSON_Delete(root);
    
    return ESP_OK;
}

esp_err_t web_server_start(bool is_ap_mode) {
    if (server != NULL) {
        ESP_LOGW(TAG, "Web server already running");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.core_id = 0;  // Run on Core 0
    config.task_priority = 5;
    config.stack_size = 8192;
    config.max_uri_handlers = 10;

    ESP_LOGI(TAG, "Starting web server on Core 0 (AP mode: %s)", is_ap_mode ? "yes" : "no");

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server");
        return ESP_FAIL;
    }

    // Register URI handlers
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &root_uri);

    if (is_ap_mode) {
        httpd_uri_t save_uri = {
            .uri = "/save",
            .method = HTTP_POST,
            .handler = save_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &save_uri);
    }

    httpd_uri_t status_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &status_uri);

    ESP_LOGI(TAG, "Web server started successfully");
    return ESP_OK;
}

esp_err_t web_server_stop(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "Web server stopped");
    }
    return ESP_OK;
}
