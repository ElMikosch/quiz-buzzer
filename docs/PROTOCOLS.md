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

---

## WebSocket Protocol (Main Controller → PC/Mobile Clients)

The main controller provides a WebSocket server for bidirectional JSON-based communication over WiFi. This is the recommended protocol for modern applications, especially mobile and web-based quiz software.

### Connection Details

- **Port**: 8080 (TCP)
- **Protocol**: WebSocket (RFC 6455)
- **Format**: JSON messages
- **Discovery**: mDNS hostname `quizbuzzer.local` (when in Station Mode)
- **Max Clients**: 4 concurrent connections
- **Heartbeat**: Ping every 5 seconds, timeout after 2 seconds without pong

### Connection URLs

**Station Mode** (connected to your WiFi network):
```
ws://quizbuzzer.local:8080
ws://<assigned-ip>:8080
```

**Setup Mode** (QuizBuzzer-Setup AP):
```
ws://192.168.4.1:8080
```

### Outbound Messages (Controller → Client)

All messages are broadcast to all connected WebSocket clients simultaneously. Each message includes a `timestamp` field with `millis()` value for event ordering.

#### Buzzer Press Event
```json
{
  "type": "buzzer",
  "id": 2,
  "timestamp": 12345
}
```

#### Correct Answer Event
```json
{
  "type": "correct",
  "timestamp": 12350
}
```

#### Wrong Answer Event
```json
{
  "type": "wrong",
  "timestamp": 12355
}
```

#### Reset Event
```json
{
  "type": "reset",
  "timestamp": 12360
}
```

#### Buzzer Disconnect Event
```json
{
  "type": "disconnect",
  "id": 3,
  "timestamp": 12365
}
```

#### Buzzer Reconnect Event
```json
{
  "type": "reconnect",
  "id": 3,
  "timestamp": 12370
}
```

#### State Synchronization Message
Sent immediately when a client connects, and whenever requested via `getState` command.

```json
{
  "type": "state",
  "selected": 2,
  "locked": [1, 4],
  "timestamp": 12375
}
```

Fields:
- `selected`: Currently selected buzzer (0 = none, 1-4 = buzzer ID)
- `locked`: Array of locked-out buzzer IDs
- `timestamp`: Current system time in milliseconds

### Inbound Commands (Client → Controller)

Clients send JSON commands to control the game state. Commands are processed identically to physical button presses and USB serial commands.

#### Mark Answer Correct
```json
{
  "command": "correct"
}
```

**Response:**
```json
{
  "status": "ack",
  "command": "correct",
  "timestamp": 12380
}
```

**Behavior:** Resets game to ready state (all LEDs on, no lockouts)

#### Mark Answer Wrong
```json
{
  "command": "wrong"
}
```

**Response:**
```json
{
  "status": "ack",
  "command": "wrong",
  "timestamp": 12385
}
```

**Behavior:** Locks out currently selected buzzer, returns to ready state for others

#### Reset Game
```json
{
  "command": "reset"
}
```

**Response:**
```json
{
  "status": "ack",
  "command": "reset",
  "timestamp": 12390
}
```

**Behavior:** Full reset (clears lockouts, returns all buzzers to ready state)

#### Query Current State
```json
{
  "command": "getState"
}
```

**Response:** (State message as shown above)
```json
{
  "type": "state",
  "selected": 0,
  "locked": [],
  "timestamp": 12395
}
```

#### Error Response
Sent when command is invalid or malformed:
```json
{
  "status": "error",
  "message": "Invalid command format"
}
```

### Connection Management

**Heartbeat**: The server sends WebSocket ping frames every 5 seconds. Clients must respond with pong frames within 2 seconds to maintain the connection.

**Auto-Disconnect**: Clients that fail to respond to ping are automatically disconnected after timeout.

**Reconnection**: Clients can reconnect at any time. Upon connection, they immediately receive a state sync message with the current game state.

### JavaScript Example

```javascript
const ws = new WebSocket('ws://quizbuzzer.local:8080');

ws.onopen = () => {
  console.log('Connected to quiz buzzer');
  
  // Request current state
  ws.send(JSON.stringify({ command: 'getState' }));
};

ws.onmessage = (event) => {
  const msg = JSON.parse(event.data);
  
  switch(msg.type) {
    case 'state':
      console.log(`Selected: ${msg.selected}, Locked: ${msg.locked}`);
      break;
    case 'buzzer':
      console.log(`Buzzer ${msg.id} pressed!`);
      break;
    case 'correct':
      console.log('Correct answer!');
      break;
    case 'wrong':
      console.log('Wrong answer!');
      break;
    case 'reset':
      console.log('Game reset');
      break;
    case 'disconnect':
      console.log(`Buzzer ${msg.id} disconnected`);
      break;
    case 'reconnect':
      console.log(`Buzzer ${msg.id} reconnected`);
      break;
  }
};

ws.onerror = (error) => {
  console.error('WebSocket error:', error);
};

ws.onclose = () => {
  console.log('Disconnected from quiz buzzer');
  // Implement auto-reconnect logic here
};

// Send commands
function markCorrect() {
  ws.send(JSON.stringify({ command: 'correct' }));
}

function markWrong() {
  ws.send(JSON.stringify({ command: 'wrong' }));
}

function resetGame() {
  ws.send(JSON.stringify({ command: 'reset' }));
}
```

### Python Example (websocket-client library)

```python
import websocket
import json

def on_message(ws, message):
    msg = json.loads(message)
    msg_type = msg.get('type')
    
    if msg_type == 'state':
        print(f"State - Selected: {msg['selected']}, Locked: {msg['locked']}")
    elif msg_type == 'buzzer':
        print(f"Buzzer {msg['id']} pressed!")
    elif msg_type == 'correct':
        print("Correct answer!")
    elif msg_type == 'wrong':
        print("Wrong answer!")
    elif msg_type == 'reset':
        print("Game reset")
    elif msg_type == 'disconnect':
        print(f"Buzzer {msg['id']} disconnected")
    elif msg_type == 'reconnect':
        print(f"Buzzer {msg['id']} reconnected")

def on_open(ws):
    print("Connected to quiz buzzer")
    # Request current state
    ws.send(json.dumps({"command": "getState"}))

def on_error(ws, error):
    print(f"Error: {error}")

def on_close(ws, close_status_code, close_msg):
    print("Disconnected from quiz buzzer")

# Connect to controller
ws = websocket.WebSocketApp(
    "ws://quizbuzzer.local:8080",
    on_open=on_open,
    on_message=on_message,
    on_error=on_error,
    on_close=on_close
)

ws.run_forever()

# In another thread, send commands:
# ws.send(json.dumps({"command": "correct"}))
# ws.send(json.dumps({"command": "wrong"}))
# ws.send(json.dumps({"command": "reset"}))
```

### Godot GDScript Example (with Auto-Reconnect)

```gdscript
extends Node

# WebSocket client for Quiz Buzzer system
var socket = WebSocketPeer.new()
var connection_url = "ws://quizbuzzer.local:8080"
var is_connected = false
var reconnect_timer = 0.0
const RECONNECT_INTERVAL = 3.0  # Try reconnecting every 3 seconds

# Game state tracking
var selected_buzzer = 0
var locked_buzzers = []

# Signals for game events
signal buzzer_pressed(id)
signal correct_answer()
signal wrong_answer()
signal game_reset()
signal state_updated(selected, locked)
signal buzzer_disconnected(id)
signal buzzer_reconnected(id)

func _ready():
	connect_to_server()

func _process(delta):
	socket.poll()
	
	var state = socket.get_ready_state()
	
	if state == WebSocketPeer.STATE_OPEN:
		if not is_connected:
			is_connected = true
			print("✓ Connected to Quiz Buzzer!")
			request_state()
		
		# Process incoming messages
		while socket.get_available_packet_count() > 0:
			var packet = socket.get_packet()
			var json_string = packet.get_string_from_utf8()
			var json = JSON.new()
			var parse_result = json.parse(json_string)
			
			if parse_result == OK:
				handle_message(json.data)
			else:
				print("JSON parse error: ", json.get_error_message())
	
	elif state == WebSocketPeer.STATE_CLOSED:
		if is_connected:
			print("✗ Disconnected from Quiz Buzzer")
			is_connected = false
		
		# Auto-reconnect logic
		reconnect_timer += delta
		if reconnect_timer >= RECONNECT_INTERVAL:
			reconnect_timer = 0.0
			connect_to_server()

func connect_to_server():
	print("Connecting to ", connection_url, "...")
	var err = socket.connect_to_url(connection_url)
	if err != OK:
		print("Connection error: ", err)

func handle_message(data: Dictionary):
	var msg_type = data.get("type", "")
	
	match msg_type:
		"state":
			selected_buzzer = data.get("selected", 0)
			locked_buzzers = data.get("locked", [])
			print("State sync - Selected: ", selected_buzzer, ", Locked: ", locked_buzzers)
			emit_signal("state_updated", selected_buzzer, locked_buzzers)
		
		"buzzer":
			var id = data.get("id", 0)
			print("Buzzer ", id, " pressed!")
			selected_buzzer = id
			emit_signal("buzzer_pressed", id)
		
		"correct":
			print("Answer marked correct!")
			selected_buzzer = 0
			locked_buzzers = []
			emit_signal("correct_answer")
		
		"wrong":
			print("Answer marked wrong!")
			if selected_buzzer > 0 and not selected_buzzer in locked_buzzers:
				locked_buzzers.append(selected_buzzer)
			selected_buzzer = 0
			emit_signal("wrong_answer")
		
		"reset":
			print("Game reset!")
			selected_buzzer = 0
			locked_buzzers = []
			emit_signal("game_reset")
		
		"disconnect":
			var id = data.get("id", 0)
			print("Buzzer ", id, " disconnected!")
			emit_signal("buzzer_disconnected", id)
		
		"reconnect":
			var id = data.get("id", 0)
			print("Buzzer ", id, " reconnected!")
			emit_signal("buzzer_reconnected", id)

func send_command(command: String):
	if not is_connected:
		print("Cannot send command - not connected")
		return
	
	var data = {"command": command}
	var json_string = JSON.stringify(data)
	socket.send_text(json_string)
	print("Sent command: ", command)

# Public API for controlling the game
func mark_correct():
	send_command("correct")

func mark_wrong():
	send_command("wrong")

func reset_game():
	send_command("reset")

func request_state():
	send_command("getState")
```

**Usage in Your Game:**

```gdscript
# In your main scene
extends Node2D

@onready var buzzer_client = $QuizBuzzerClient

func _ready():
	# Connect to signals
	buzzer_client.buzzer_pressed.connect(_on_buzzer_pressed)
	buzzer_client.correct_answer.connect(_on_correct_answer)
	buzzer_client.wrong_answer.connect(_on_wrong_answer)
	buzzer_client.game_reset.connect(_on_game_reset)
	buzzer_client.state_updated.connect(_on_state_updated)

func _on_buzzer_pressed(id):
	print("Player ", id, " buzzed in!")
	# Update UI, play sound, etc.

func _on_correct_answer():
	print("Correct! Moving to next question...")
	# Award points, advance game state

func _on_wrong_answer():
	print("Wrong answer, player locked out")
	# Update UI to show locked players

func _on_game_reset():
	print("Game reset by host")
	# Reset UI, clear selections

func _on_state_updated(selected, locked):
	print("Game state: Selected=", selected, " Locked=", locked)
	# Update UI to match current state

# Call these when host presses buttons
func _on_correct_button_pressed():
	buzzer_client.mark_correct()

func _on_wrong_button_pressed():
	buzzer_client.mark_wrong()

func _on_reset_button_pressed():
	buzzer_client.reset_game()
```

### Dual-Interface Operation

The WebSocket and USB Serial interfaces operate concurrently:
- Events are broadcast to **both** interfaces simultaneously
- Commands from either interface have identical effects
- No interference between interfaces
- USB Serial remains unchanged for backward compatibility
