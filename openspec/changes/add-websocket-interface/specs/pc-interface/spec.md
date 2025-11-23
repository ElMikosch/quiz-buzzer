## ADDED Requirements

### Requirement: WebSocket Connection Protocol
Client applications SHALL connect to the main controller via WebSocket on port 8080.

#### Scenario: WebSocket URL format
- **WHEN** a client application wants to connect
- **THEN** it SHALL use WebSocket URL: ws://192.168.4.1:8080
- **AND** establish connection after joining "QuizBuzzer" WiFi network
- **AND** use standard WebSocket protocol (RFC 6455)

#### Scenario: Initial WiFi connection
- **WHEN** a client device (phone, tablet, laptop) searches for WiFi networks
- **THEN** it SHALL discover "QuizBuzzer" SSID
- **AND** connect using password "buzzer1234" (or configured password)
- **AND** receive IP address 192.168.4.x via DHCP
- **AND** have gateway 192.168.4.1

#### Scenario: WebSocket handshake
- **WHEN** client initiates WebSocket connection
- **THEN** it SHALL send HTTP upgrade request with WebSocket headers
- **AND** receive 101 Switching Protocols response
- **AND** establish full-duplex WebSocket connection
- **AND** be ready to send and receive JSON messages

#### Scenario: Multiple simultaneous connections
- **WHEN** up to 4 clients connect simultaneously
- **THEN** all SHALL be able to establish WebSocket connections
- **AND** all SHALL receive identical game event broadcasts
- **AND** any client SHALL be able to send control commands

### Requirement: JSON Message Format
WebSocket messages SHALL use JSON format for structured, type-safe communication.

#### Scenario: Outbound message structure
- **WHEN** the main controller sends a message
- **THEN** it SHALL be valid JSON with "type" field
- **AND** include "timestamp" field with millis() value
- **AND** include additional fields based on message type
- **AND** be newline-terminated for stream parsing

#### Scenario: Buzzer press JSON message
- **WHEN** a buzzer is pressed
- **THEN** clients SHALL receive JSON: {"type": "buzzer", "id": 1, "timestamp": 1234567}
- **AND** "id" SHALL be 1-4 indicating which buzzer
- **AND** "timestamp" SHALL be controller's millis() value

#### Scenario: Correct answer JSON message
- **WHEN** correct answer action is executed
- **THEN** clients SHALL receive JSON: {"type": "correct", "timestamp": 1234567}

#### Scenario: Wrong answer JSON message
- **WHEN** wrong answer action is executed
- **THEN** clients SHALL receive JSON: {"type": "wrong", "timestamp": 1234567}

#### Scenario: Reset JSON message
- **WHEN** reset action is executed
- **THEN** clients SHALL receive JSON: {"type": "reset", "timestamp": 1234567}

#### Scenario: Disconnect event JSON message
- **WHEN** a buzzer node disconnects
- **THEN** clients SHALL receive JSON: {"type": "disconnect", "id": 2, "timestamp": 1234567}
- **AND** "id" SHALL indicate which buzzer node disconnected

#### Scenario: Reconnect event JSON message
- **WHEN** a buzzer node reconnects
- **THEN** clients SHALL receive JSON: {"type": "reconnect", "id": 3, "timestamp": 1234567}
- **AND** "id" SHALL indicate which buzzer node reconnected

#### Scenario: State sync JSON message
- **WHEN** a client connects or requests state
- **THEN** it SHALL receive JSON: {"type": "state", "selected": 0, "locked": [], "timestamp": 1234567}
- **AND** "selected" SHALL be 0-4 (0=none, 1-4=buzzer ID)
- **AND** "locked" SHALL be array of locked buzzer IDs (empty array if none)

### Requirement: JSON Command Protocol
Client applications SHALL send control commands to the main controller using JSON format.

#### Scenario: Send CORRECT command
- **WHEN** client wants to mark answer correct
- **THEN** it SHALL send JSON: {"command": "correct"}
- **AND** receive acknowledgment: {"status": "ack", "command": "correct", "timestamp": ...}
- **AND** game state SHALL transition identically to USB serial CORRECT command
- **AND** all connected clients SHALL receive broadcast event

#### Scenario: Send WRONG command
- **WHEN** client wants to mark answer wrong
- **THEN** it SHALL send JSON: {"command": "wrong"}
- **AND** receive acknowledgment: {"status": "ack", "command": "wrong", "timestamp": ...}
- **AND** game state SHALL transition identically to USB serial WRONG command

#### Scenario: Send RESET command
- **WHEN** client wants to reset game
- **THEN** it SHALL send JSON: {"command": "reset"}
- **AND** receive acknowledgment: {"status": "ack", "command": "reset", "timestamp": ...}
- **AND** game state SHALL transition identically to USB serial RESET command

#### Scenario: Request current game state
- **WHEN** client wants to query current state
- **THEN** it SHALL send JSON: {"command": "getState"}
- **AND** receive response: {"type": "state", "selected": <id>, "locked": [<ids>], "timestamp": ...}
- **AND** NOT trigger any game state changes

#### Scenario: Command acknowledgment timing
- **WHEN** a client sends any command
- **THEN** it SHALL receive acknowledgment within 50ms
- **AND** command SHALL be executed within 10ms
- **AND** broadcast event SHALL be sent to all clients within 20ms

#### Scenario: Invalid command format
- **WHEN** client sends malformed JSON or missing "command" field
- **THEN** it SHALL receive error response: {"status": "error", "message": "Invalid command format"}
- **AND** connection SHALL remain open
- **AND** game state SHALL NOT change

#### Scenario: Command with no buzzer selected
- **WHEN** client sends "correct" or "wrong" command
- **AND** no buzzer is currently selected
- **THEN** it SHALL receive: {"status": "ack", "command": "<cmd>", "ignored": true, "reason": "No buzzer selected"}
- **AND** game state SHALL NOT change

### Requirement: WebSocket Connection Management
Client applications SHALL implement connection management and reconnection logic.

#### Scenario: Connection establishment
- **WHEN** client connects successfully
- **THEN** it SHALL immediately receive state sync message
- **AND** begin receiving real-time event broadcasts
- **AND** be able to send commands immediately

#### Scenario: Connection lost detection
- **WHEN** WebSocket connection is lost (network issue, controller reset)
- **THEN** client SHALL detect disconnection via close event or ping timeout
- **AND** attempt automatic reconnection after 2 seconds
- **AND** display "Disconnected" status to user

#### Scenario: Automatic reconnection
- **WHEN** connection is restored after disconnection
- **THEN** client SHALL re-establish WebSocket connection
- **AND** receive new state sync message
- **AND** resume normal operation
- **AND** display "Connected" status to user

#### Scenario: Ping/pong heartbeat handling
- **WHEN** client receives WebSocket ping frame
- **THEN** it SHALL respond with pong frame within 1 second
- **AND** maintain connection alive
- **WHEN** client does not receive ping for 10 seconds
- **THEN** it SHALL assume connection lost and attempt reconnection

### Requirement: Godot Integration
The WebSocket protocol SHALL be compatible with Godot engine's built-in WebSocket client (GDScript, C#, GDNative).

#### Scenario: Godot WebSocket client connection
- **WHEN** Godot application uses WebSocketClient.connect_to_url("ws://192.168.4.1:8080")
- **THEN** connection SHALL establish successfully
- **AND** _on_connection_established signal SHALL fire
- **AND** Godot app SHALL receive state sync message immediately

#### Scenario: Godot message reception
- **WHEN** main controller broadcasts JSON event
- **THEN** Godot client SHALL receive via _on_data_received signal
- **AND** message SHALL be parseable via JSON.parse() or JSON.parse_string()
- **AND** parsed dictionary SHALL contain "type" key and event-specific fields

#### Scenario: Godot command sending
- **WHEN** Godot app sends command via send_text(JSON.stringify({"command": "correct"}))
- **THEN** main controller SHALL receive and process command
- **AND** Godot app SHALL receive acknowledgment message
- **AND** Godot app SHALL receive broadcast event via _on_data_received

#### Scenario: Godot mobile export compatibility
- **WHEN** Godot app is exported to iOS or Android
- **THEN** WebSocket connection SHALL work identically to desktop
- **AND** no platform-specific code SHALL be required
- **AND** WiFi connection SHALL use native OS WiFi APIs

#### Scenario: Godot error handling
- **WHEN** WebSocket connection fails or closes
- **THEN** Godot app SHALL receive _on_connection_closed or _on_connection_error signal
- **AND** be able to implement reconnection logic in GDScript
- **AND** display appropriate UI feedback to user

### Requirement: Multi-Client Coordination
Multiple WebSocket clients SHALL be able to connect and interact with the quiz system concurrently.

#### Scenario: Multiple clients receive same events
- **WHEN** 2 or more clients are connected
- **AND** a game event occurs (buzzer press, control action)
- **THEN** all clients SHALL receive identical JSON broadcast
- **AND** messages SHALL arrive within 50ms of each other
- **AND** no client SHALL be blocked by slow clients

#### Scenario: Commands from different clients
- **WHEN** client A sends "correct" command
- **AND** client B sends "wrong" command simultaneously
- **THEN** main controller SHALL process commands in reception order (FIFO)
- **AND** both clients SHALL receive acknowledgments for their commands
- **AND** all clients SHALL receive broadcast events for both actions

#### Scenario: Mixed USB serial and WebSocket usage
- **WHEN** USB serial is sending commands
- **AND** WebSocket clients are connected
- **THEN** USB serial commands SHALL be processed identically
- **AND** WebSocket clients SHALL receive broadcasts for all events (including serial-triggered)
- **AND** USB serial SHALL receive messages for all events (including WebSocket-triggered)
- **AND** neither interface SHALL block the other

### Requirement: Backward Compatibility
The WebSocket interface SHALL coexist with the existing USB serial interface without breaking changes.

#### Scenario: USB serial continues to function
- **WHEN** WebSocket server is active
- **THEN** USB serial SHALL continue to send all existing messages
- **AND** message format SHALL remain unchanged (BUZZER:1\n, CORRECT\n, etc.)
- **AND** serial baud rate SHALL remain 115200
- **AND** existing PC software using serial SHALL work without modification

#### Scenario: Serial and WebSocket concurrent operation
- **WHEN** both USB serial and WebSocket clients are active
- **THEN** both interfaces SHALL receive all game events
- **AND** commands from either interface SHALL trigger broadcasts to both
- **AND** system SHALL operate normally with either or both interfaces

#### Scenario: WebSocket disabled via build flag
- **WHEN** firmware is compiled with -DDISABLE_WEBSOCKET flag
- **THEN** WebSocket and WiFi code SHALL be excluded
- **AND** USB serial interface SHALL function identically to previous version
- **AND** no WiFi AP SHALL be created
- **AND** firmware size SHALL be reduced by ~60KB

### Requirement: Cross-Platform Client Support
The WebSocket protocol SHALL be accessible from any platform with WebSocket support.

#### Scenario: Web browser clients
- **WHEN** accessing WebSocket from JavaScript in browser
- **THEN** connection SHALL establish via: new WebSocket("ws://192.168.4.1:8080")
- **AND** messages SHALL be parseable via JSON.parse()
- **AND** commands SHALL be sendable via ws.send(JSON.stringify({...}))

#### Scenario: Python client
- **WHEN** using Python websocket-client library
- **THEN** connection SHALL work with standard WebSocket API
- **AND** messages SHALL be parseable via json.loads()

#### Scenario: iOS native client
- **WHEN** using iOS URLSessionWebSocketTask
- **THEN** connection SHALL establish after joining QuizBuzzer WiFi
- **AND** Swift/Objective-C apps SHALL be able to integrate

#### Scenario: Android native client
- **WHEN** using Android OkHttp WebSocket client
- **THEN** connection SHALL establish after joining QuizBuzzer WiFi
- **AND** Java/Kotlin apps SHALL be able to integrate

#### Scenario: Unity engine compatibility
- **WHEN** using Unity with WebSocket plugin (UnityWebSocket, NativeWebSocket)
- **THEN** connection and messaging SHALL work identically to Godot
- **AND** enable Unity mobile quiz apps

### Requirement: Performance and Latency
WebSocket communication SHALL maintain acceptable performance for real-time quiz gameplay.

#### Scenario: WebSocket message latency
- **WHEN** a buzzer is pressed
- **THEN** WebSocket clients SHALL receive JSON message within 100ms
- **AND** latency SHALL be consistently <100ms under normal conditions
- **AND** latency SHALL be acceptable for human-perceptible gameplay

#### Scenario: WebSocket does not degrade USB serial
- **WHEN** WebSocket server is active with connected clients
- **THEN** USB serial message latency SHALL remain <10ms
- **AND** USB serial SHALL NOT experience increased latency or dropped messages

#### Scenario: High-frequency events
- **WHEN** multiple rapid events occur (rapid buzzer presses, rapid control actions)
- **THEN** all WebSocket clients SHALL receive all events in order
- **AND** message queue SHALL buffer up to 10 messages per client
- **AND** oldest messages SHALL be dropped if queue exceeds 10

### Requirement: Error Handling and Diagnostics
The WebSocket interface SHALL provide clear error messages and diagnostic information.

#### Scenario: Connection refused diagnostic
- **WHEN** client cannot connect to WebSocket
- **THEN** typical causes SHALL be documented:
  - Not connected to "QuizBuzzer" WiFi network
  - Incorrect IP address (should be 192.168.4.1)
  - Incorrect port (should be 8080)
  - Main controller not powered on or WiFi initialization failed

#### Scenario: Invalid JSON diagnostic
- **WHEN** client sends invalid JSON
- **THEN** error response SHALL include specific reason
- **AND** example valid format SHALL be documented

#### Scenario: Debug logging via USB serial
- **WHEN** WebSocket events occur (connection, disconnection, commands)
- **THEN** main controller SHALL log events to USB serial with "DEBUG: WS:" prefix
- **AND** allow troubleshooting via serial monitor while WebSocket is active
