# Communication Protocols

## ESP-NOW Protocol (Buzzer Nodes ↔ Main Controller)

### Custom MAC Addresses

The system uses programmatically-set custom MAC addresses for predictable peer identification:

- **Main Controller**: `AA:BB:CC:DD:EE:00`
- **Buzzer Node 1**: `AA:BB:CC:DD:EE:01`
- **Buzzer Node 2**: `AA:BB:CC:DD:EE:02`
- **Buzzer Node 3**: `AA:BB:CC:DD:EE:03`
- **Buzzer Node 4**: `AA:BB:CC:DD:EE:04`

These are set at startup via `esp_wifi_set_mac()`. No manual MAC address configuration is required.

### Message Structure

```cpp
struct BuzzerMessage {
  uint8_t node_id;      // 1-4 for buzzer nodes, 0 for main controller
  uint8_t msg_type;     // MSG_BUTTON_PRESS, MSG_LED_COMMAND, MSG_ACK
  uint8_t value;        // LED state or press count
  uint32_t timestamp;   // millis() for deduplication
};
```

### Message Types

| Type | Value | Direction | Description |
|------|-------|-----------|-------------|
| MSG_BUTTON_PRESS | 1 | Buzzer → Main | Button pressed on buzzer node |
| MSG_LED_COMMAND | 2 | Main → Buzzer | LED control command |
| MSG_ACK | 3 | Bidirectional | Acknowledgment (future use) |

### LED States

| State | Value | Description |
|-------|-------|-------------|
| LED_OFF | 0 | LED off (locked out) |
| LED_ON | 1 | LED solid on (ready/active) |
| LED_BLINK | 2 | LED blinking at 2Hz (selected) |

### Message Flow Examples

#### Button Press
1. User presses button on Buzzer Node 2
2. Node 2 sends `BuzzerMessage{node_id=2, msg_type=MSG_BUTTON_PRESS, value=1, timestamp=...}` to main controller
3. Retry up to 3 times with 10ms interval if transmission fails
4. Main controller processes press and updates game state

#### LED Control
1. Main controller determines new LED state for Buzzer Node 3
2. Main sends `BuzzerMessage{node_id=3, msg_type=MSG_LED_COMMAND, value=LED_BLINK, timestamp=...}`
3. Node 3 receives and updates its LED state
4. No acknowledgment required (fire-and-forget)

### Communication Parameters

- **WiFi Channel**: 1 (configurable in config.h)
- **Encryption**: Disabled (no pairing needed)
- **Typical Latency**: 10-30ms
- **Max Range**: ~50m (line of sight)

---

## USB Serial Protocol (Main Controller → PC)

### Connection Parameters

- **Baud Rate**: 115200
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Flow Control**: None

### Message Format

All messages are newline-terminated ASCII text (`\n` = 0x0A).

| Message | Description | Example |
|---------|-------------|---------|
| `BUZZER:<id>\n` | Buzzer pressed | `BUZZER:1\n` |
| `CORRECT\n` | Correct answer button pressed | `CORRECT\n` |
| `WRONG\n` | Wrong answer button pressed | `WRONG\n` |
| `RESET\n` | Reset button pressed | `RESET\n` |

### Reading Serial Messages (Python Example)

```python
import serial

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

while True:
    line = ser.readline().decode('utf-8').strip()
    if line:
        if line.startswith('BUZZER:'):
            buzzer_id = int(line.split(':')[1])
            print(f"Buzzer {buzzer_id} pressed!")
        elif line == 'CORRECT':
            print("Correct answer!")
        elif line == 'WRONG':
            print("Wrong answer!")
        elif line == 'RESET':
            print("System reset!")
```

### Reading Serial Messages (Node.js Example)

```javascript
const SerialPort = require('serialport');
const Readline = require('@serialport/parser-readline');

const port = new SerialPort('/dev/ttyUSB0', { baudRate: 115200 });
const parser = port.pipe(new Readline({ delimiter: '\n' }));

parser.on('data', line => {
  if (line.startsWith('BUZZER:')) {
    const buzzerId = parseInt(line.split(':')[1]);
    console.log(`Buzzer ${buzzerId} pressed!`);
  } else if (line === 'CORRECT') {
    console.log('Correct answer!');
  } else if (line === 'WRONG') {
    console.log('Wrong answer!');
  } else if (line === 'RESET') {
    console.log('System reset!');
  }
});
```

### Message Queue

The main controller maintains a queue of up to 10 messages to handle rapid events without loss. Messages are sent in FIFO order during each loop iteration.

### Debug Output

Additional debug messages may appear on the serial port (e.g., "Buzzer node ready!", "ERROR: ..."). Quiz software should filter these by looking for the defined message patterns.
