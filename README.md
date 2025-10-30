# Quiz Buzzer System

A distributed ESP32-based quiz buzzer system with 4 wireless buzzer nodes and a main controller for quiz game management.

## Features

- **4 Independent Buzzer Nodes**: Wireless ESP32-based buttons with LED feedback
- **First-Press Detection**: Reliable distributed timing across all buzzers
- **Game State Management**: Ready, locked, and partial-lockout states
- **Auto-Reconnection**: Automatic state recovery after power cycle or network issues
- **Connection Monitoring**: Heartbeat system detects disconnections within 5 seconds
- **Answer Validation**: Correct/wrong/reset controls for game host
- **PC Integration**: USB serial interface (115200 baud) for quiz software
- **Visual Feedback**: LED states (solid=ready, blink=selected, off=locked, rapid blink=disconnected)
- **Low Latency**: Sub-100ms response time via ESP-NOW protocol
- **Battery Powered Buzzers**: ~4-6 hours operation with light sleep mode

## Hardware

- 5x Lolin32 Lite (ESP32) boards
- 7x Push buttons
- 4x LEDs with resistors
- 4x 1100mAh 3.7V LiPo batteries
- 1x USB cable

## Quick Start

### 1. Flash Firmware
```bash
# Flash main controller
pio run -e main_controller -t upload

# Flash each buzzer node
pio run -e buzzer_node_1 -t upload
pio run -e buzzer_node_2 -t upload
pio run -e buzzer_node_3 -t upload
pio run -e buzzer_node_4 -t upload
```

### 2. Label Boards
Physically label each board after flashing:
- Main controller: "MAIN"
- Buzzer nodes: "Buzzer 1" through "Buzzer 4"

### 3. Test System
1. Power main controller first
2. Power all buzzer nodes
3. All LEDs should turn on (ready state)
4. Press any buzzer → that LED blinks, others turn off
5. Press CORRECT button → all LEDs turn on again

### 4. Connect to PC
```bash
# Linux/Mac
screen /dev/ttyUSB0 115200

# Windows
# Use PuTTY or Arduino Serial Monitor at 115200 baud
```

## Documentation

- **[GPIO Pins](docs/GPIO_PINS.md)** - Pin assignments and hardware requirements
- **[Protocols](docs/PROTOCOLS.md)** - ESP-NOW and USB Serial protocols
- **[State Machine](docs/STATE_MACHINE.md)** - Game states and transitions
- **[Deployment](docs/DEPLOYMENT.md)** - Full deployment and troubleshooting guide

## PC Interface

Messages sent over USB serial (newline-terminated):
```
BUZZER:1      # Buzzer 1 pressed
BUZZER:2      # Buzzer 2 pressed
BUZZER:3      # Buzzer 3 pressed
BUZZER:4      # Buzzer 4 pressed
CORRECT       # Correct answer button
WRONG         # Wrong answer button
RESET         # Reset button
DISCONNECT:2  # Buzzer 2 disconnected
RECONNECT:2   # Buzzer 2 reconnected
```

Example Python code:
```python
import serial
ser = serial.Serial('/dev/ttyUSB0', 115200)
while True:
    line = ser.readline().decode().strip()
    if line.startswith('BUZZER:'):
        buzzer_id = int(line.split(':')[1])
        print(f"Buzzer {buzzer_id} pressed!")
```

## Project Structure

```
quiz-buzzer/
├── src/
│   ├── buzzer_node.cpp    # Buzzer node firmware
│   ├── controller.cpp     # Main controller firmware
│   ├── protocol.h         # Shared message protocol
│   ├── config.h           # Pin assignments and constants
│   └── main.cpp           # Entry point (empty, routing via platformio.ini)
├── docs/
│   ├── GPIO_PINS.md       # Hardware pin assignments
│   ├── PROTOCOLS.md       # Communication protocols
│   ├── STATE_MACHINE.md   # Game state documentation
│   └── DEPLOYMENT.md      # Deployment and troubleshooting
├── openspec/              # Design proposals and specs
└── platformio.ini         # Build configurations
```

## Building

### Prerequisites
- [PlatformIO](https://platformio.org/install)
- USB drivers for Lolin32 Lite (CH340 or CP2102)

### Build Commands
```bash
# Build all environments
pio run

# Build specific environment
pio run -e main_controller
pio run -e buzzer_node_1

# Build and upload
pio run -e main_controller -t upload

# Monitor serial output
pio device monitor -e main_controller
```

## Troubleshooting

### Buzzer LEDs Not Responding
- Verify custom MAC addresses in serial output (AA:BB:CC:DD:EE:00-04)
- Check ESP-NOW initialization messages
- Ensure main controller powered first
- Reduce distance between boards

### Button Presses Not Detected
- Test button continuity with multimeter
- Check GPIO pin assignments match wiring
- Verify internal pullups enabled (default)

### No Serial Messages to PC
- Check baud rate is 115200
- Verify USB drivers installed
- Check serial port permissions (Linux: add user to `dialout` group)
- Ensure main controller receiving buzzer presses (check local serial)

### Buzzer Shows Rapid Blinking (10Hz)
- This indicates disconnection from main controller
- Check main controller is powered and running
- Verify both devices on same WiFi channel (channel 1)
- Move buzzer closer to main controller (within ~20m)
- Check serial output on main controller for `DISCONNECT:<id>` messages
- Buzzer will auto-reconnect when heartbeat resumes

### Buzzer Reconnects But Wrong LED State
- System automatically syncs state on reconnection
- Check serial output for `STATE_SYNC` messages
- If issue persists, press RESET button on main controller to clear all state

### Build Errors
- Ensure PlatformIO is updated: `pio upgrade`
- Clean build: `pio run -t clean`
- Check that correct environment is selected

See **[docs/DEPLOYMENT.md](docs/DEPLOYMENT.md)** for detailed troubleshooting.

## Future Enhancements

- [ ] Speaker audio feedback on buzzer nodes (GPIO 14 reserved)
- [ ] Low battery voltage monitoring
- [ ] Visual display screen
- [ ] Over-the-air (OTA) firmware updates
- [ ] DIP switch node ID configuration
- [ ] Auto-discovery of buzzer nodes

## License

MIT License - See LICENSE file for details

## Contributing

This project uses OpenSpec for change proposals. See `openspec/AGENTS.md` for guidelines on creating proposals.

## Support

For issues and questions, open an issue on GitHub.
