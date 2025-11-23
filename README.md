# Quiz Buzzer System

A distributed ESP32-based quiz buzzer system with 4 wireless buzzer nodes and a main controller for quiz game management.

## Features

- **4 Independent Buzzer Nodes**: Wireless ESP32-based buttons with LED feedback
- **First-Press Detection**: Reliable distributed timing across all buzzers
- **Game State Management**: Ready, locked, and partial-lockout states
- **Auto-Reconnection**: Automatic state recovery after power cycle or network issues
- **Connection Monitoring**: Heartbeat system detects disconnections within 5 seconds
- **Answer Validation**: Correct/wrong/reset controls for game host
- **WiFi Configuration**: Easy setup via captive portal web interface
- **WebSocket Interface**: JSON-based API for modern web/mobile apps (ws://quizbuzzer.local:8080)
- **USB Serial Interface**: Traditional text-based protocol for PC integration (115200 baud)
- **Dual Interface**: WebSocket and USB Serial work simultaneously without interference
- **mDNS Discovery**: Connect via `quizbuzzer.local` hostname (no IP address needed)
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

### 4. WiFi Setup (Optional)

The controller can operate in two modes:

**AP Setup Mode** (default on first boot):
1. Look for WiFi network "QuizBuzzer-Setup" on your phone/laptop
2. Connect to it (open network, no password)
3. Captive portal should auto-open, or go to http://192.168.4.1
4. Select your WiFi network and enter password
5. Controller will reboot and connect to your network

**Station Mode** (after WiFi configured):
- Controller connects to your WiFi network
- Access via mDNS: `ws://quizbuzzer.local:8080`
- Or find IP address in your router's DHCP list

**WiFi Reset**: Hold RESET button for 3 seconds during boot to clear WiFi credentials and return to Setup Mode.

### 5. Connect to PC

**Option A: USB Serial** (traditional method)
```bash
# Linux/Mac
screen /dev/ttyUSB0 115200

# Windows
# Use PuTTY or Arduino Serial Monitor at 115200 baud
```

**Option B: WebSocket** (recommended for modern apps)
```javascript
// Connect via mDNS (Station Mode)
const ws = new WebSocket('ws://quizbuzzer.local:8080');

// Or via IP in Setup Mode
const ws = new WebSocket('ws://192.168.4.1:8080');

ws.onmessage = (event) => {
  const msg = JSON.parse(event.data);
  console.log('Received:', msg);
};
```

See [docs/PROTOCOLS.md](docs/PROTOCOLS.md) for complete WebSocket API documentation.

## Documentation

- **[GPIO Pins](docs/GPIO_PINS.md)** - Pin assignments and hardware requirements
- **[Protocols](docs/PROTOCOLS.md)** - ESP-NOW and USB Serial protocols
- **[State Machine](docs/STATE_MACHINE.md)** - Game states and transitions
- **[Deployment](docs/DEPLOYMENT.md)** - Full deployment and troubleshooting guide

## PC/Mobile Interface

The controller supports **two communication interfaces** that work simultaneously:
- **WebSocket** (JSON): Modern, recommended for web/mobile apps
- **USB Serial** (text): Traditional, for desktop PC applications

### WebSocket Interface (Recommended)

**Connection:**
```javascript
const ws = new WebSocket('ws://quizbuzzer.local:8080');
```

**Receive Events:**
```javascript
ws.onmessage = (event) => {
  const msg = JSON.parse(event.data);
  
  switch(msg.type) {
    case 'buzzer':    // Buzzer pressed: {type: "buzzer", id: 1-4, timestamp: ...}
    case 'correct':   // Correct button: {type: "correct", timestamp: ...}
    case 'wrong':     // Wrong button: {type: "wrong", timestamp: ...}
    case 'reset':     // Reset button: {type: "reset", timestamp: ...}
    case 'state':     // Full state: {type: "state", selected: 0-4, locked: [ids], timestamp: ...}
    case 'disconnect': // Buzzer lost: {type: "disconnect", id: 1-4, timestamp: ...}
    case 'reconnect':  // Buzzer back: {type: "reconnect", id: 1-4, timestamp: ...}
  }
};
```

**Send Commands:**
```javascript
ws.send(JSON.stringify({ command: 'correct' }));  // Mark answer correct
ws.send(JSON.stringify({ command: 'wrong' }));    // Mark answer wrong
ws.send(JSON.stringify({ command: 'reset' }));    // Reset game
ws.send(JSON.stringify({ command: 'getState' })); // Query current state
```

See **[docs/PROTOCOLS.md](docs/PROTOCOLS.md#websocket-protocol-main-controller--pcmobile-clients)** for complete WebSocket API documentation with Python and JavaScript examples.

### USB Serial Interface

**Outbound Messages** (Controller → PC, newline-terminated):
```
BUZZER:1      # Buzzer 1 pressed
BUZZER:2      # Buzzer 2 pressed
BUZZER:3      # Buzzer 3 pressed
BUZZER:4      # Buzzer 4 pressed
CORRECT       # Correct answer button pressed
WRONG         # Wrong answer button pressed
RESET         # Reset button pressed
DISCONNECT:2  # Buzzer 2 disconnected
RECONNECT:2   # Buzzer 2 reconnected
```

**Inbound Commands** (PC → Controller):
```
CORRECT\n     # Mark answer correct and reset game
WRONG\n       # Mark answer wrong and lock out current buzzer
RESET\n       # Full reset of game state
```

**Command Responses:**
```
CMD_ACK:CORRECT           # Command acknowledged and executed
CMD_ACK:WRONG             # Command acknowledged and executed
CMD_ACK:RESET             # Command acknowledged and executed
CMD_ERR:UNKNOWN:FOO       # Unknown command "FOO"
CMD_ERR:BUFFER_OVERFLOW   # Input exceeded 256 bytes
```

### Example: Reading Messages
```python
import serial
ser = serial.Serial('/dev/ttyUSB0', 115200)
while True:
    line = ser.readline().decode().strip()
    if line.startswith('BUZZER:'):
        buzzer_id = int(line.split(':')[1])
        print(f"Buzzer {buzzer_id} pressed!")
    elif line == 'CORRECT':
        print("Correct answer!")
```

### Example: Sending Commands
```python
import serial
ser = serial.Serial('/dev/ttyUSB0', 115200)

# Mark answer correct
ser.write(b'CORRECT\n')
response = ser.readline().decode().strip()
print(response)  # CMD_ACK:CORRECT

# Mark answer wrong
ser.write(b'WRONG\n')
response = ser.readline().decode().strip()
print(response)  # CMD_ACK:WRONG

# Reset game
ser.write(b'RESET\n')
response = ser.readline().decode().strip()
print(response)  # CMD_ACK:RESET
```

### Using with Serial Terminal
```bash
# Linux/Mac - using screen
screen /dev/ttyUSB0 115200
# Type commands: CORRECT, WRONG, or RESET followed by Enter

# Alternative - using echo
echo "CORRECT" > /dev/ttyUSB0

# Windows PowerShell
"CORRECT`n" | Out-File -FilePath COM3 -Encoding ASCII -NoNewline
```

**Note**: Serial commands work identically to physical button presses and can be used concurrently.

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

### WiFi and WebSocket Issues

#### Can't Find "QuizBuzzer-Setup" Network
- Ensure main controller is powered on and in Setup Mode (first boot or after WiFi reset)
- Check controller serial output for "Setup Mode" message
- Force Setup Mode: Hold RESET button for 3 seconds during boot

#### Can't Connect to WebSocket (quizbuzzer.local)
- Verify controller is in Station Mode (connected to your WiFi)
- Check serial output shows "WiFi Connected" with IP address
- Try connecting via IP instead: `ws://<ip-address>:8080`
- Ensure your device is on the same WiFi network as the controller
- Some routers block mDNS - use IP address directly

#### WebSocket Disconnects Frequently
- Check WiFi signal strength (move controller closer to router)
- Verify router is not on a congested channel
- Check for interference from other 2.4GHz devices
- Controller sends ping every 5 seconds - ensure client responds to pong

#### WiFi Credentials Not Saving
- Ensure SSID and password are correct
- Check serial output for NVS write errors
- Try WiFi reset and reconfigure: Hold RESET button 3 seconds during boot

#### Controller Stuck in Setup Mode
- This happens when WiFi connection fails
- Verify WiFi password is correct
- Check that WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Ensure router is not filtering MAC addresses

### ESP-NOW and Buzzer Issues

#### Buzzer LEDs Not Responding
- Verify custom MAC addresses in serial output (AA:BB:CC:DD:EE:00-04)
- Check ESP-NOW initialization messages
- Ensure main controller powered first
- Reduce distance between boards

#### Button Presses Not Detected
- Test button continuity with multimeter
- Check GPIO pin assignments match wiring
- Verify internal pullups enabled (default)

#### No Serial Messages to PC
- Check baud rate is 115200
- Verify USB drivers installed
- Check serial port permissions (Linux: add user to `dialout` group)
- Ensure main controller receiving buzzer presses (check local serial)

#### Buzzer Shows Rapid Blinking (10Hz)
- This indicates disconnection from main controller
- Check main controller is powered and running
- Verify both devices on same WiFi channel (channel 1 in Setup Mode)
- Move buzzer closer to main controller (within ~20m)
- Check serial output on main controller for `DISCONNECT:<id>` messages
- Buzzer will auto-reconnect when heartbeat resumes

#### Buzzer Reconnects But Wrong LED State
- System automatically syncs state on reconnection
- Check serial output for `STATE_SYNC` messages
- If issue persists, press RESET button on main controller to clear all state

#### ESP-NOW Stops Working After WiFi Connect
- Controller automatically synchronizes ESP-NOW to WiFi channel
- If router changes channels, ESP-NOW adapts automatically
- Buzzer nodes follow main controller's channel

### Build Issues

#### Build Errors
- Ensure PlatformIO is updated: `pio upgrade`
- Clean build: `pio run -t clean`
- Check that correct environment is selected

#### Missing Libraries
- Run `pio lib install` to install dependencies
- Check platformio.ini for required libraries

See **[docs/DEPLOYMENT.md](docs/DEPLOYMENT.md)** for detailed troubleshooting.

## WiFi Configuration Details

### First Boot Setup

1. Flash and power on the main controller
2. It will create WiFi AP "QuizBuzzer-Setup" (no password)
3. Connect with phone/tablet/laptop
4. Captive portal opens automatically (or navigate to http://192.168.4.1)
5. Select your WiFi network from the scanned list
6. Enter WiFi password
7. Click "Save & Connect"
8. Controller reboots and connects to your network

### Station Mode Operation

Once configured, the controller:
- Connects to your WiFi network on boot
- Announces itself via mDNS as `quizbuzzer.local`
- Runs WebSocket server on port 8080
- Falls back to Setup Mode if connection fails for 30 seconds

### Reconfiguring WiFi

**Method 1: Web Interface (from Station Mode)**
- Navigate to http://quizbuzzer.local or http://<controller-ip>
- Click "Reset WiFi" to return to Setup Mode

**Method 2: Physical Button**
- Hold RESET button for 3 seconds during boot
- Controller clears credentials and enters Setup Mode

### Access Methods

| Mode | WebSocket URL | Web Interface |
|------|--------------|---------------|
| Setup Mode | `ws://192.168.4.1:8080` | http://192.168.4.1 |
| Station Mode | `ws://quizbuzzer.local:8080` | http://quizbuzzer.local |
| Station Mode (IP) | `ws://<assigned-ip>:8080` | http://<assigned-ip> |

## Future Enhancements

- [ ] Speaker audio feedback on buzzer nodes (GPIO 14 reserved)
- [ ] Low battery voltage monitoring
- [ ] Visual display screen
- [ ] Over-the-air (OTA) firmware updates via web interface
- [ ] DIP switch node ID configuration
- [ ] Auto-discovery of buzzer nodes
- [ ] WebSocket authentication (bearer tokens)

## License

MIT License - See LICENSE file for details

## Contributing

This project uses OpenSpec for change proposals. See `openspec/AGENTS.md` for guidelines on creating proposals.

## Support

For issues and questions, open an issue on GitHub.
