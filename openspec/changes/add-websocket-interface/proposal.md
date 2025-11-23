# Add WebSocket Interface for Mobile/Cross-Platform Apps

## Why

The current USB serial interface requires physical cable connection and platform-specific serial port handling, making it difficult to integrate with mobile apps (iOS/Android) and cross-platform frameworks like Godot. WebSocket provides a modern, wireless, and widely-supported protocol that works seamlessly across web browsers, mobile apps, and desktop applications without special drivers or permissions.

## What Changes

- Add WiFi Access Point mode to main controller for standalone operation
- Implement WebSocket server on main controller (port 8080) for bidirectional communication
- Support JSON-based message protocol alongside existing text-based serial protocol
- Maintain full backward compatibility with existing USB serial interface
- Enable concurrent connections from multiple clients (e.g., Godot app + debug console)
- Provide connection status monitoring and automatic reconnection handling

## Impact

- **Affected specs**: main-controller, pc-interface
- **Affected code**: 
  - `src/controller.cpp` - Add WiFi AP setup and WebSocket server
  - `src/config.h` - Add WiFi/WebSocket configuration constants
  - `platformio.ini` - Add WebSocket library dependency
- **Benefits**:
  - Enables Godot mobile/desktop app development
  - Removes need for USB cable during gameplay
  - Allows multiple simultaneous connections (scoreboard + controller)
  - Simplifies cross-platform integration
- **Trade-offs**:
  - Adds ~40KB flash memory usage for WiFi/WebSocket libraries
  - Minimal power consumption increase (main controller is USB-powered)
  - Requires initial WiFi configuration (credentials or AP mode)
