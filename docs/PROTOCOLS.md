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
  uint8_t msg_type;     // MSG_BUTTON_PRESS, MSG_LED_COMMAND, MSG_ACK, MSG_HEARTBEAT, MSG_STATE_REQUEST, MSG_STATE_SYNC
  uint8_t value;        // LED state, press count, or packed game state
  uint32_t timestamp;   // millis() for deduplication
};
```

### Message Types

| Type | Value | Direction | Description |
|------|-------|-----------|-------------|
| MSG_BUTTON_PRESS | 1 | Buzzer → Main | Button pressed on buzzer node |
| MSG_LED_COMMAND | 2 | Main → Buzzer | LED control command |
| MSG_ACK | 3 | Bidirectional | Acknowledgment (future use) |
| MSG_HEARTBEAT | 4 | Main → Buzzers | Periodic heartbeat broadcast (every 2s) |
| MSG_STATE_REQUEST | 5 | Buzzer → Main | Request game state after reconnection |
| MSG_STATE_SYNC | 6 | Main → Buzzer | Full game state synchronization |

### LED States

| State | Value | Description |
|-------|-------|-------------|
| LED_OFF | 0 | LED off (locked out) |
| LED_ON | 1 | LED solid on (ready/active) |
| LED_BLINK | 2 | LED blinking at 2Hz (selected) or 10Hz (disconnected) |

### Connection Monitoring & Recovery

The system includes automatic connection monitoring and state recovery:

**Heartbeat Mechanism:**
- Main controller broadcasts `MSG_HEARTBEAT` every 2 seconds to all buzzers
- Buzzer nodes track time since last heartbeat received
- If no heartbeat for 5 seconds, buzzer enters "disconnected" state
- Disconnected state indicated by rapid LED blink (10Hz / 100ms interval)

**Reconnection Flow:**
1. Buzzer detects heartbeat after timeout (power cycle or network issue)
2. Buzzer sends `MSG_STATE_REQUEST` to main controller
3. Main controller responds with `MSG_STATE_SYNC` containing current game state
4. Buzzer unpacks state and restores correct LED behavior

**State Sync Message Format (value field):**
- Bits 0-3: `lockedBuzzers` bitmask (bit 0 = buzzer 1, bit 1 = buzzer 2, etc.)
- Bits 4-6: `selectedBuzzer` (0 = none, 1-4 = buzzer ID)
- Bit 7: unused

**Connection Timing Constants:**
- `HEARTBEAT_INTERVAL_MS`: 2000 (2 seconds)
- `CONNECTION_TIMEOUT_MS`: 5000 (5 seconds)
- `RECONNECT_GRACE_PERIOD_MS`: 1000 (1 second after reconnect to process state sync)
- `DISCONNECT_BLINK_INTERVAL_MS`: 100 (rapid blink during disconnection)

**Serial Logging:**
- `DISCONNECT:<node_id>` - Node has timed out
- `RECONNECT:<node_id>` - Node has reconnected
- `STATE_SYNC:<node_id> (state=..., selected=..., locked=...)` - State sync sent

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

#### Heartbeat & Connection Monitoring
1. Main controller broadcasts `BuzzerMessage{node_id=0, msg_type=MSG_HEARTBEAT, value=0, timestamp=...}` every 2 seconds
2. All buzzer nodes receive and update their last-heartbeat timestamp
3. If a buzzer doesn't receive heartbeat for 5 seconds, it enters disconnected state (rapid LED blink)
4. Main controller tracks last-seen timestamp for each node; logs `DISCONNECT:<id>` if timeout detected

#### Reconnection & State Sync
1. Buzzer node detects heartbeat after being disconnected (power cycled or network restored)
2. Node sends `BuzzerMessage{node_id=X, msg_type=MSG_STATE_REQUEST, value=0, timestamp=...}`
3. Main controller logs `RECONNECT:<id>` and sends `MSG_STATE_SYNC` with packed game state
4. Node unpacks state (locked bitmask + selected buzzer) and restores correct LED state

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
| `DISCONNECT:<id>\n` | Buzzer node has disconnected (timeout) | `DISCONNECT:2\n` |
| `RECONNECT:<id>\n` | Buzzer node has reconnected | `RECONNECT:2\n` |
| `STATE_SYNC:<id> (...)\n` | State sync sent to reconnected node (debug) | `STATE_SYNC:2 (state=1, selected=1, locked=0x0)\n` |

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
