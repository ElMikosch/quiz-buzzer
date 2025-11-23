## ADDED Requirements

### Requirement: WiFi Configuration System
The main controller SHALL support runtime WiFi configuration without firmware reflashing.

#### Scenario: First boot (unconfigured state)
- **WHEN** the main controller powers on for the first time
- **OR** WiFi credentials are not stored in NVS
- **THEN** it SHALL start in AP Setup Mode
- **AND** create WiFi Access Point with SSID "QuizBuzzer-Setup" (open, no password)
- **AND** start DNS server for captive portal redirect
- **AND** start HTTP server on port 80 with configuration web interface
- **AND** display "Setup Mode" status on serial console

#### Scenario: WiFi configuration web interface
- **WHEN** device is in AP Setup Mode
- **AND** user connects to "QuizBuzzer-Setup" network
- **THEN** HTTP requests SHALL redirect to 192.168.4.1 (captive portal)
- **AND** web interface SHALL display:
  - List of scanned WiFi networks (SSID + signal strength)
  - Password input field
  - "Save & Connect" button
  - Current connection status
- **AND** support WiFi network scan refresh

#### Scenario: Saving WiFi credentials
- **WHEN** user submits SSID and password via web interface
- **THEN** controller SHALL validate credentials are not empty
- **AND** store SSID in NVS with key "wifi_ssid"
- **AND** store password in NVS with key "wifi_pass"
- **AND** persist data across power cycles
- **AND** respond with success confirmation
- **AND** reboot into Station Mode within 2 seconds

#### Scenario: Station Mode connection
- **WHEN** WiFi credentials exist in NVS
- **AND** controller boots normally
- **THEN** it SHALL attempt connection to configured WiFi network
- **AND** wait up to 30 seconds for connection
- **AND** display connection status on serial console
- **AND** IF successful: start WebSocket server and announce via mDNS
- **AND** IF failed: fall back to AP Setup Mode

#### Scenario: mDNS service announcement
- **WHEN** connected in Station Mode
- **THEN** controller SHALL announce mDNS hostname "quizbuzzer.local"
- **AND** advertise WebSocket service on port 8080
- **AND** respond to mDNS queries
- **AND** allow clients to connect via ws://quizbuzzer.local:8080

#### Scenario: Manual WiFi reset via physical button
- **WHEN** user holds RESET button (GPIO 27) during boot
- **AND** button is held for 3 seconds continuously
- **THEN** controller SHALL clear WiFi credentials from NVS
- **AND** boot into AP Setup Mode
- **AND** display "WiFi Reset - Entering Setup Mode" message on serial console
- **NOTE**: This check only occurs during setup() initialization, normal game reset functionality is unaffected

#### Scenario: Automatic fallback to AP Setup Mode
- **WHEN** in Station Mode
- **AND** WiFi connection fails or is lost
- **AND** reconnection attempts fail for 30 seconds
- **THEN** controller SHALL revert to AP Setup Mode
- **AND** display "Connection Lost - Setup Mode" on serial console
- **AND** allow user to reconfigure WiFi credentials

#### Scenario: WiFi reconfiguration from Station Mode
- **WHEN** in Station Mode
- **AND** user accesses web interface at http://quizbuzzer.local or http://<station_ip>
- **THEN** web interface SHALL allow changing WiFi credentials
- **AND** display current SSID (password hidden)
- **AND** allow "Forget Network" action (clears NVS, returns to AP Setup Mode)
- **AND** allow "Scan Networks" to find new networks

### Requirement: WiFi Station Mode Operation
The main controller SHALL join existing WiFi networks to enable internet access for connected devices.

#### Scenario: WiFi Station initialization
- **WHEN** WiFi credentials are configured in NVS
- **AND** the main controller powers on
- **THEN** it SHALL initialize WiFi in Station mode
- **AND** connect to configured SSID using stored password
- **AND** obtain IP address via DHCP
- **AND** use WiFi channel determined by router
- **AND** complete connection within 30 seconds or fallback to AP Setup Mode

#### Scenario: Station Mode IP discovery
- **WHEN** connected in Station Mode
- **THEN** controller SHALL log assigned IP address to serial console
- **AND** format: "WiFi Connected: <SSID> | IP: <address> | mDNS: quizbuzzer.local"
- **AND** update web interface to display current IP
- **AND** announce via mDNS as "quizbuzzer.local"

#### Scenario: ESP-NOW channel synchronization in Station Mode
- **WHEN** connected to router in Station Mode
- **AND** router operates on WiFi channel X
- **THEN** ESP-NOW SHALL also operate on channel X
- **AND** buzzer nodes SHALL automatically follow channel change
- **AND** maintain <30ms buzzer latency

### Requirement: WiFi Access Point Setup Mode
The main controller SHALL create a WiFi Access Point for initial configuration and fallback operation.

#### Scenario: AP Setup Mode initialization
- **WHEN** the main controller enters AP Setup Mode
- **THEN** it SHALL initialize WiFi in Access Point mode
- **AND** set SSID to "QuizBuzzer-Setup"
- **AND** set open network (no password for easy first-time setup)
- **AND** use IP address 192.168.4.1
- **AND** use WiFi channel 1 (shared with ESP-NOW)
- **AND** allow up to 4 simultaneous client connections
- **AND** complete initialization within 3 seconds

#### Scenario: WiFi channel compatibility with ESP-NOW
- **WHEN** WiFi AP is active
- **AND** ESP-NOW is active on channel 1
- **THEN** both protocols SHALL operate simultaneously without interference
- **AND** buzzer node communication SHALL maintain <30ms latency
- **AND** WebSocket communication SHALL maintain <100ms latency

#### Scenario: AP beacon broadcasting
- **WHEN** WiFi AP is running
- **THEN** it SHALL broadcast SSID beacon every 100ms
- **AND** be discoverable by WiFi-capable devices (phones, tablets, laptops)
- **AND** accept WPA2-PSK authentication

#### Scenario: Client IP assignment
- **WHEN** a client connects to the WiFi AP
- **THEN** the main controller SHALL assign IP via DHCP
- **AND** assign IPs in range 192.168.4.2 to 192.168.4.10
- **AND** set gateway to 192.168.4.1
- **AND** allow immediate WebSocket connection attempts

### Requirement: WebSocket Server
The main controller SHALL run a WebSocket server on port 8080 for bidirectional communication.

#### Scenario: WebSocket server initialization
- **WHEN** WiFi AP is active
- **THEN** the WebSocket server SHALL start on port 8080
- **AND** listen for incoming connections
- **AND** accept WebSocket handshake (RFC 6455)
- **AND** support up to 4 concurrent client connections

#### Scenario: Client WebSocket connection
- **WHEN** a client initiates WebSocket handshake to ws://192.168.4.1:8080
- **THEN** the server SHALL accept the connection
- **AND** perform HTTP upgrade handshake
- **AND** add client to active connections list
- **AND** send full state sync message immediately
- **AND** log connection event with client IP

#### Scenario: Client WebSocket disconnection
- **WHEN** a WebSocket client disconnects (graceful close or timeout)
- **THEN** the server SHALL remove client from active connections list
- **AND** log disconnection event with client IP
- **AND** continue serving other connected clients

#### Scenario: WebSocket ping/pong heartbeat
- **WHEN** a WebSocket client is connected
- **THEN** the server SHALL send ping frame every 5 seconds
- **AND** expect pong response within 2 seconds
- **AND** close connection if no pong received (client timeout)

### Requirement: JSON Message Broadcasting
The main controller SHALL broadcast game events to all connected WebSocket clients using JSON format.

#### Scenario: Buzzer press WebSocket broadcast
- **WHEN** a buzzer button press is registered
- **THEN** the controller SHALL broadcast JSON message to all WebSocket clients
- **AND** use format: {"type": "buzzer", "id": <1-4>, "timestamp": <millis>}
- **AND** send within 20ms of button press registration
- **AND** also send traditional serial message (backward compatibility)

#### Scenario: Control button WebSocket broadcast
- **WHEN** a control button action is executed (correct/wrong/reset)
- **THEN** the controller SHALL broadcast corresponding JSON message
- **AND** use format: {"type": "correct"|"wrong"|"reset", "timestamp": <millis>}
- **AND** send to all WebSocket clients concurrently

#### Scenario: Connection event WebSocket broadcast
- **WHEN** a buzzer node disconnects or reconnects
- **THEN** the controller SHALL broadcast JSON message
- **AND** use format: {"type": "disconnect"|"reconnect", "id": <1-4>, "timestamp": <millis>}
- **AND** send to all WebSocket clients

#### Scenario: Concurrent broadcast to multiple clients
- **WHEN** multiple WebSocket clients are connected
- **AND** a game event occurs
- **THEN** the message SHALL be sent to all clients concurrently
- **AND** each client SHALL receive identical JSON message
- **AND** message delivery failure to one client SHALL NOT block others

### Requirement: JSON Command Processing
The main controller SHALL accept and process control commands from WebSocket clients in JSON format.

#### Scenario: Receive CORRECT command via WebSocket
- **WHEN** a WebSocket client sends {"command": "correct"}
- **AND** a buzzer is currently selected
- **THEN** the controller SHALL execute handleCorrectAnswer()
- **AND** respond with {"status": "ack", "command": "correct", "timestamp": <millis>}
- **AND** execute identical behavior to physical CORRECT button
- **AND** process within 10ms

#### Scenario: Receive WRONG command via WebSocket
- **WHEN** a WebSocket client sends {"command": "wrong"}
- **AND** a buzzer is currently selected
- **THEN** the controller SHALL execute handleWrongAnswer()
- **AND** respond with {"status": "ack", "command": "wrong", "timestamp": <millis>}
- **AND** execute identical behavior to physical WRONG button
- **AND** process within 10ms

#### Scenario: Receive RESET command via WebSocket
- **WHEN** a WebSocket client sends {"command": "reset"}
- **THEN** the controller SHALL execute handleReset()
- **AND** respond with {"status": "ack", "command": "reset", "timestamp": <millis>}
- **AND** execute identical behavior to physical RESET button
- **AND** process within 10ms

#### Scenario: Receive getState command via WebSocket
- **WHEN** a WebSocket client sends {"command": "getState"}
- **THEN** the controller SHALL respond with current game state
- **AND** use format: {"type": "state", "selected": <0-4>, "locked": [<ids>], "timestamp": <millis>}
- **AND** selected=0 means no buzzer selected
- **AND** locked array contains IDs of locked-out buzzers

#### Scenario: Invalid JSON command format
- **WHEN** a WebSocket client sends malformed JSON or missing "command" field
- **THEN** the controller SHALL respond with {"status": "error", "message": "Invalid command format"}
- **AND** NOT change game state
- **AND** continue normal operation

#### Scenario: Command from WebSocket with no buzzer selected
- **WHEN** receiving "correct" or "wrong" command
- **AND** no buzzer is currently selected (selectedBuzzer == 0)
- **THEN** the controller SHALL ignore the command
- **AND** respond with {"status": "ack", "command": "<cmd>", "ignored": true, "reason": "No buzzer selected"}
- **AND** NOT change game state

### Requirement: Unified Message Broadcasting
The main controller SHALL broadcast all game events to both USB serial and WebSocket interfaces concurrently.

#### Scenario: Dual-interface broadcast
- **WHEN** any game event occurs (buzzer press, control action, connection event)
- **THEN** the controller SHALL format and send message to USB serial
- **AND** format and broadcast JSON message to all WebSocket clients
- **AND** both interfaces SHALL receive semantically identical information
- **AND** neither interface SHALL block the other

#### Scenario: Interface-independent operation
- **WHEN** USB serial is disconnected
- **THEN** WebSocket interface SHALL continue to function normally
- **WHEN** no WebSocket clients are connected
- **THEN** USB serial interface SHALL continue to function normally

### Requirement: WebSocket State Synchronization
The main controller SHALL send full game state to WebSocket clients upon connection.

#### Scenario: State sync on WebSocket connection
- **WHEN** a new WebSocket client connects
- **THEN** the controller SHALL immediately send state sync message
- **AND** use format: {"type": "state", "selected": <0-4>, "locked": [<ids>], "timestamp": <millis>}
- **AND** selected SHALL reflect currently selected buzzer (0 if none)
- **AND** locked array SHALL contain all locked-out buzzer IDs
- **AND** send before any other event messages

#### Scenario: State sync with buzzer selected
- **WHEN** a WebSocket client connects
- **AND** buzzer 2 is currently selected
- **AND** buzzer 3 is locked out
- **THEN** the state message SHALL be {"type": "state", "selected": 2, "locked": [3], "timestamp": ...}

#### Scenario: State sync in ready state
- **WHEN** a WebSocket client connects
- **AND** game is in ready state (no selection, no lockouts)
- **THEN** the state message SHALL be {"type": "state", "selected": 0, "locked": [], "timestamp": ...}

### Requirement: WebSocket Performance and Reliability
WebSocket communication SHALL be reliable and maintain acceptable latency for quiz gameplay.

#### Scenario: WebSocket message latency
- **WHEN** a game event occurs
- **THEN** JSON message SHALL be sent to WebSocket clients within 20ms
- **AND** be received by client application within 100ms (including network latency)

#### Scenario: WebSocket does not degrade ESP-NOW performance
- **WHEN** WebSocket server is active with connected clients
- **THEN** ESP-NOW message latency SHALL remain <30ms
- **AND** buzzer press detection SHALL maintain <100ms end-to-end latency
- **AND** LED command transmission SHALL not be delayed

#### Scenario: WebSocket buffer management
- **WHEN** a WebSocket client is slow to consume messages
- **THEN** the server SHALL buffer up to 10 messages per client
- **AND** drop oldest messages if buffer exceeds 10
- **AND** log buffer overflow event
- **AND** NOT block other clients or USB serial interface

#### Scenario: Non-blocking WebSocket operation
- **WHEN** WebSocket operations are in progress (send/receive)
- **THEN** the main loop SHALL NOT block
- **AND** ESP-NOW message reception SHALL continue
- **AND** button press handling SHALL continue with <10ms response time
