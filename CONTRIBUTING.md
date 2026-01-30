# Contributing to ESP-NTP

Thank you for your interest in contributing to ESP-NTP!

## Development Setup

1. Install [PlatformIO](https://platformio.org/)
2. Clone the repository
3. Build: `pio run`
4. Upload: `pio run --target upload`

## Code Style

- Follow ESP-IDF coding conventions
- Use `snake_case` for functions
- Use `UPPER_SNAKE_CASE` for constants
- Add descriptive comments for public APIs
- Keep functions focused and modular

## Component Structure

Each component should have:
- `CMakeLists.txt` - Build configuration
- `include/` directory with public headers
- Source files implementing the component
- Clear separation of concerns

## Testing

- Test on actual hardware (XIAO ESP32-S3)
- Verify with GPS module (ATGM336H)
- Test PPS signal accuracy
- Validate NVS checksum handling
- Test AP mode configuration flow

## Pull Request Process

1. Create a feature branch
2. Make your changes with clear commit messages
3. Test on hardware
4. Submit PR with description of changes
5. Address review feedback

## Questions?

Open an issue for discussion before major changes.
