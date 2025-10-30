# Project Context

## Purpose
Quiz buzzer system built for ESP32 microcontroller. Designed to handle multiple buzzer inputs for quiz games, with real-time response detection and feedback.

## Tech Stack
- **Platform**: PlatformIO
- **Framework**: Arduino
- **Hardware**: ESP32 (Lolin32 Lite)
- **Language**: C++
- **Build System**: PlatformIO/SCons
- **Tooling**: clangd, clang-tidy for code analysis

## Project Conventions

### Code Style
- C++ with Arduino framework conventions
- Follow standard Arduino naming (setup(), loop() for entry points)
- Use descriptive variable names for GPIO pins and states
- Keep embedded code simple and efficient (memory-constrained environment)
- Use Serial output for debugging at 9600 baud

### Architecture Patterns
- Standard Arduino architecture with setup() and loop()
- Hardware abstraction for buzzer inputs
- Event-driven input handling for button presses
- State machine patterns for game logic (pending, active, locked states)

### Testing Strategy
- Manual testing on hardware
- Serial output for debugging and monitoring
- Unit testing where applicable for pure logic functions

### Git Workflow
- Standard feature branch workflow
- Descriptive commit messages focusing on "why" not "what"
- Test on hardware before committing functional changes

## Domain Context
- Quiz buzzer system: multiple players compete to press their buzzer first
- First press detection with lockout to prevent subsequent presses
- Visual/audio feedback for the first player
- Reset mechanism to start new questions
- Real-time requirements: must detect and respond to button presses within milliseconds

## Important Constraints
- Limited memory on ESP32 (320KB RAM, 4MB flash on Lolin32 Lite)
- Real-time constraints for buzzer detection (sub-100ms response time)
- Power consumption considerations for embedded system
- GPIO pin limitations
- Serial communication at 9600 baud for debugging

## External Dependencies
- Arduino framework libraries
- ESP32 platform libraries (espressif32)
- Hardware-specific libraries for GPIO, timers, interrupts
