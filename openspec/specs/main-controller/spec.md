# main-controller Specification

## Purpose
Defines the behavior and responsibilities of the main controller board, which acts as the ESP-NOW hub, game state coordinator, physical control button interface, and bidirectional USB serial gateway for PC communication.
## Requirements
### Requirement: ESP-NOW Hub
The main controller SHALL receive messages from all 4 buzzer nodes using ESP-NOW.

#### Scenario: Receive button press from buzzer node
- **WHEN** a buzzer node transmits a button press message
- **THEN** the main controller SHALL receive the message
- **AND** extract the node ID and timestamp
- **AND** process the button press according to current game state

#### Scenario: Handle simultaneous messages
- **WHEN** multiple buzzer nodes transmit within 1ms
- **THEN** the main controller SHALL process messages in reception order
- **AND** use reception timestamp (micros()) for first-press determination

#### Scenario: Send LED command to buzzer node
- **WHEN** the game state changes
- **THEN** the main controller SHALL send LED commands to affected buzzer nodes
- **AND** use broadcast for "all LEDs on/off" commands
- **AND** use unicast for specific node LED commands

### Requirement: Control Button Interface
The main controller SHALL provide 3 physical buttons for game control.

#### Scenario: Correct answer button pressed
- **WHEN** the game master presses the "correct answer" button (button 1)
- **AND** a buzzer is currently selected
- **THEN** the controller SHALL send 'C' key to PC
- **AND** reset game state to ready
- **AND** turn on all buzzer LEDs

#### Scenario: Wrong answer button pressed
- **WHEN** the game master presses the "wrong answer" button (button 2)
- **AND** a buzzer is currently selected
- **THEN** the controller SHALL send 'W' key to PC
- **AND** lock out the currently selected buzzer
- **AND** turn off the locked buzzer's LED
- **AND** allow remaining buzzers to press again
- **AND** turn on LEDs for remaining active buzzers

#### Scenario: Reset button pressed
- **WHEN** the game master presses the "reset" button (button 3)
- **THEN** the controller SHALL send 'R' key to PC
- **AND** clear all game state (unlock all buzzers)
- **AND** turn on all buzzer LEDs
- **AND** return to ready state

#### Scenario: Control button pressed in invalid state
- **WHEN** correct or wrong button is pressed
- **AND** no buzzer is currently selected
- **THEN** the controller SHALL ignore the button press
- **AND** NOT send any key to PC

### Requirement: USB Serial Communication
The main controller SHALL send event messages to the PC via USB serial port.

#### Scenario: Buzzer press serial output
- **WHEN** a buzzer button press is registered
- **THEN** the controller SHALL send "BUZZER:<id>\n" to serial port
- **AND** <id> SHALL be the buzzer number (1-4)
- **AND** the message SHALL be newline-terminated

#### Scenario: Control button serial output
- **WHEN** a control button is pressed and action is taken
- **THEN** the controller SHALL send the corresponding message to serial port
- **AND** correct answer SHALL send "CORRECT\n"
- **AND** wrong answer SHALL send "WRONG\n"
- **AND** reset SHALL send "RESET\n"

#### Scenario: Serial baud rate
- **WHEN** the serial port is initialized
- **THEN** the baud rate SHALL be set to 115200
- **AND** use 8 data bits, no parity, 1 stop bit (8N1)

#### Scenario: Message queuing
- **WHEN** multiple events occur rapidly
- **THEN** messages SHALL be queued (up to 10 messages)
- **AND** sent in FIFO order
- **AND** oldest message discarded if queue is full

### Requirement: Serial Command Input Processing
The main controller SHALL accept and process control commands received via USB serial port from the PC, validating that commands are only executed in valid game states.

#### Scenario: Process CORRECT command from serial (MODIFIED)
- **WHEN** receiving "CORRECT\n" or "CORRECT\r\n" from serial port
- **AND** a buzzer is currently selected (selectedBuzzer != 0)
- **THEN** the controller SHALL call handleCorrectAnswer()
- **AND** send "CMD_ACK:CORRECT\n" acknowledgment
- **AND** execute identical behavior to physical CORRECT button press
- **AND** process the command within 10ms

#### Scenario: Process CORRECT command with no buzzer selected (ADDED)
- **WHEN** receiving "CORRECT\n" or "CORRECT\r\n" from serial port
- **AND** no buzzer is currently selected (selectedBuzzer == 0)
- **THEN** the controller SHALL ignore the command
- **AND** send "CMD_ACK:CORRECT\n" acknowledgment
- **AND** log "No buzzer selected, ignoring CORRECT command" to Serial
- **AND** NOT change game state
- **AND** NOT send any message to PC interface

#### Scenario: Process WRONG command from serial (MODIFIED)
- **WHEN** receiving "WRONG\n" or "WRONG\r\n" from serial port
- **AND** a buzzer is currently selected (selectedBuzzer != 0)
- **THEN** the controller SHALL call handleWrongAnswer()
- **AND** send "CMD_ACK:WRONG\n" acknowledgment
- **AND** execute identical behavior to physical WRONG button press
- **AND** process the command within 10ms

#### Scenario: Process WRONG command with no buzzer selected (ADDED)
- **WHEN** receiving "WRONG\n" or "WRONG\r\n" from serial port
- **AND** no buzzer is currently selected (selectedBuzzer == 0)
- **THEN** the controller SHALL ignore the command
- **AND** send "CMD_ACK:WRONG\n" acknowledgment
- **AND** log "No buzzer selected, ignoring WRONG command" to Serial
- **AND** NOT change game state
- **AND** NOT send any message to PC interface

### Requirement: Power-On Initialization
The main controller SHALL initialize all subsystems on power-up.

#### Scenario: Successful initialization
- **WHEN** the controller powers on
- **THEN** it SHALL initialize GPIO pins for 3 control buttons (with pullups)
- **AND** initialize ESP-NOW as receiver for 4 buzzer nodes
- **AND** initialize USB serial at 115200 baud
- **AND** broadcast "all LEDs on" command to buzzer nodes
- **AND** print initialization status to Serial at 115200 baud

#### Scenario: Initialization failure
- **WHEN** any subsystem fails to initialize
- **THEN** the controller SHALL print error details to Serial
- **AND** retry initialization every 5 seconds
- **AND** continue with partial functionality if possible

### Requirement: MAC Address Management
The main controller SHALL maintain the MAC addresses of all 4 buzzer nodes for bidirectional communication.

#### Scenario: MAC addresses configured
- **WHEN** the firmware is compiled
- **THEN** it SHALL include hardcoded MAC addresses for buzzer nodes 1-4
- **AND** use these addresses for unicast LED commands

#### Scenario: Message from unknown MAC
- **WHEN** receiving an ESP-NOW message from an unknown MAC address
- **THEN** the controller SHALL ignore the message
- **AND** log the unknown MAC to Serial

### Requirement: Watchdog and Recovery
The main controller SHALL implement watchdog timer for automatic recovery from crashes.

#### Scenario: Normal operation
- **WHEN** the controller is operating normally
- **THEN** it SHALL reset the watchdog timer every loop iteration
- **AND** the watchdog timeout SHALL be set to 5 seconds

#### Scenario: Crash or hang
- **WHEN** the watchdog timer expires (no reset for 5 seconds)
- **THEN** the ESP32 SHALL automatically reset
- **AND** reinitialize all subsystems
- **AND** return to ready state

### Requirement: Heartbeat Broadcasting
The main controller SHALL broadcast periodic heartbeat messages to all buzzer nodes.

#### Scenario: Periodic heartbeat transmission
- **WHEN** the controller is operating
- **THEN** it SHALL broadcast MSG_HEARTBEAT every 2000ms
- **AND** use ESP-NOW broadcast address (FF:FF:FF:FF:FF:FF)
- **AND** include timestamp in heartbeat message

#### Scenario: Heartbeat during all game states
- **WHEN** heartbeat interval expires
- **AND** game is in any state (ready/locked/partial lockout)
- **THEN** the heartbeat SHALL be sent
- **AND** NOT interfere with other message processing

### Requirement: Connection Status Tracking
The main controller SHALL track connection status of each buzzer node.

#### Scenario: Track last-seen timestamp
- **WHEN** receiving any message from a buzzer node
- **THEN** the controller SHALL update that node's last-seen timestamp
- **AND** mark the node as connected if previously disconnected

#### Scenario: Detect node disconnection
- **WHEN** no message received from a node for 5000ms
- **THEN** the controller SHALL mark the node as disconnected
- **AND** log disconnection event to Serial with node ID
- **AND** continue normal operation for other nodes

#### Scenario: Detect node reconnection
- **WHEN** receiving message from previously disconnected node
- **THEN** the controller SHALL mark node as connected
- **AND** log reconnection event to Serial with node ID
- **AND** wait for state sync request

### Requirement: State Synchronization Protocol
The main controller SHALL respond to state sync requests with current game state, including game state mode to distinguish between full lockout and partial lockout states.

#### Scenario: State sync message format (MODIFIED)
- **WHEN** sending MSG_STATE_SYNC to a buzzer node
- **THEN** the controller SHALL pack game state into the value field as follows:
  - Bits 0-3: locked buzzers bitmask (bit 0 = buzzer 1, bit 1 = buzzer 2, etc.)
  - Bits 4-6: selected buzzer ID (0-4, where 0 = none selected)
  - Bit 7: game state mode (0 = LOCKED/READY, 1 = PARTIAL_LOCKOUT)
- **AND** bit 7 SHALL be set to 1 when in STATE_PARTIAL_LOCKOUT
- **AND** bit 7 SHALL be set to 0 when in STATE_LOCKED or STATE_READY

#### Scenario: State sync in locked state (MODIFIED)
- **WHEN** node requests state sync
- **AND** game is in locked state
- **THEN** the controller SHALL send state sync with bit 7 = 0 (locked mode)
- **AND** send selectedBuzzer ID in bits 4-6
- **AND** send lockedBuzzers bitmask in bits 0-3 (may be 0x00)
- **AND** the buzzer node will interpret: selected buzzer gets BLINK, all others get OFF

#### Scenario: State sync in partial lockout (MODIFIED)
- **WHEN** node requests state sync
- **AND** game is in partial lockout state
- **THEN** the controller SHALL send state sync with bit 7 = 1 (partial lockout mode)
- **AND** send selectedBuzzer ID in bits 4-6
- **AND** send lockedBuzzers bitmask in bits 0-3
- **AND** the buzzer node will interpret: selected gets BLINK, locked get OFF, others get ON

#### Scenario: State sync in ready state (ADDED)
- **WHEN** node requests state sync
- **AND** game is in ready state
- **THEN** the controller SHALL send state sync with bit 7 = 0
- **AND** send selectedBuzzer = 0 in bits 4-6
- **AND** send lockedBuzzers = 0x00 in bits 0-3
- **AND** the buzzer node will interpret: all buzzers get ON

### Requirement: Reconnection Event Logging
The main controller SHALL log connection status changes via USB Serial.

#### Scenario: Log disconnection event
- **WHEN** a buzzer node disconnects (timeout)
- **THEN** the controller SHALL send "DISCONNECT:<node_id>\n" to serial
- **AND** include timestamp

#### Scenario: Log reconnection event
- **WHEN** a buzzer node reconnects
- **THEN** the controller SHALL send "RECONNECT:<node_id>\n" to serial
- **AND** include timestamp

#### Scenario: Log state sync event
- **WHEN** sending state sync to a reconnected node
- **THEN** the controller SHALL send "STATE_SYNC:<node_id>\n" to serial
- **AND** include current game state in log

### Requirement: BLE GATT Server
The main controller SHALL provide a BLE GATT server for wireless communication with PC and smartphone clients.

####Scenario: BLE initialization on power-up
- **WHEN** the controller powers on
- **THEN** it SHALL initialize BLE with device name "QuizBuzzer-XXXX" (last 4 MAC digits)
- **AND** create GATT server with Nordic UART Service UUID (6E400001-B5A3-F393-E0A9-E50E24DCCA9E)
- **AND** create TX characteristic (UUID 6E400003-B5A3-F393-E0A9-E50E24DCCA9E) with Notify + Read properties
- **AND** create RX characteristic (UUID 6E400002-B5A3-F393-E0A9-E50E24DCCA9E) with Write + Write Without Response properties
- **AND** start advertising the service
- **AND** NOT interfere with ESP-NOW initialization

#### Scenario: BLE coexists with ESP-NOW
- **WHEN** BLE is active
- **THEN** ESP-NOW communication SHALL continue to function normally
- **AND** ESP-NOW latency SHALL NOT increase by more than 10%
- **AND** ESP-NOW packet loss SHALL remain at 0%
- **AND** both radios SHALL operate simultaneously

### Requirement: BLE Game Event Notifications
The main controller SHALL send game events to connected BLE clients via characteristic notifications.

#### Scenario: Buzzer press notification via BLE
- **WHEN** a buzzer button press is registered
- **AND** a BLE client is connected
- **THEN** the controller SHALL send "BUZZER:<id>\n" via TX characteristic notification
- **AND** the message SHALL be identical to USB serial format
- **AND** the notification SHALL be sent within 50ms of the event

#### Scenario: Control button notification via BLE
- **WHEN** a control button is pressed and action is taken
- **AND** a BLE client is connected
- **THEN** the controller SHALL send corresponding message via TX characteristic notification
- **AND** correct answer SHALL send "CORRECT\n"
- **AND** wrong answer SHALL send "WRONG\n"
- **AND** reset SHALL send "RESET\n"

#### Scenario: Simultaneous USB and BLE output
- **WHEN** a game event occurs
- **THEN** the controller SHALL send the message to BOTH USB serial and BLE
- **AND** messages SHALL be identical on both transports
- **AND** neither transport SHALL block the other

### Requirement: BLE Command Input Processing
The main controller SHALL accept control commands from BLE clients via RX characteristic writes.

#### Scenario: CORRECT command received via BLE
- **WHEN** receiving "CORRECT\n" or "CORRECT" via RX characteristic write
- **THEN** the controller SHALL execute handleCorrectAnswer()
- **AND** process identically to USB serial CORRECT command
- **AND** respect game state validation (buzzer must be selected)
- **AND** process the command within 50ms

#### Scenario: WRONG command received via BLE
- **WHEN** receiving "WRONG\n" or "WRONG" via RX characteristic write
- **THEN** the controller SHALL execute handleWrongAnswer()
- **AND** process identically to USB serial WRONG command
- **AND** respect game state validation (buzzer must be selected)
- **AND** process the command within 50ms

#### Scenario: RESET command received via BLE
- **WHEN** receiving "RESET\n" or "RESET" via RX characteristic write
- **THEN** the controller SHALL execute full reset action
- **AND** process identically to USB serial RESET command
- **AND** process the command within 50ms

#### Scenario: Unknown command received via BLE
- **WHEN** receiving an unrecognized command via RX characteristic
- **THEN** the controller SHALL ignore the command
- **AND** NOT change game state
- **AND** continue normal operation

### Requirement: BLE Connection Management
The main controller SHALL manage BLE client connections and advertising.

#### Scenario: BLE client connects
- **WHEN** a BLE client connects to the GATT server
- **THEN** the controller SHALL accept the connection
- **AND** stop advertising temporarily
- **AND** log "BLE client connected" to USB serial
- **AND** enable TX characteristic notifications for this client

#### Scenario: BLE client disconnects
- **WHEN** a BLE client disconnects
- **THEN** the controller SHALL detect the disconnection
- **AND** restart advertising to accept new connections
- **AND** log "BLE client disconnected" to USB serial
- **AND** continue normal operation

#### Scenario: Single concurrent BLE connection
- **WHEN** a BLE client is already connected
- **AND** another client attempts to connect
- **THEN** the controller SHALL NOT advertise
- **AND** the second client SHALL NOT be able to connect
- **AND** the first client SHALL remain connected

### Requirement: BLE Memory Management
The main controller SHALL manage memory usage to accommodate BLE stack overhead.

#### Scenario: Memory usage with BLE active
- **WHEN** BLE is initialized and active
- **THEN** total RAM usage SHALL NOT exceed 80% of available memory
- **AND** system SHALL remain stable during 1-hour operation
- **AND** heap fragmentation SHALL NOT cause crashes

#### Scenario: Heap monitoring
- **WHEN** BLE operations are ongoing
- **THEN** free heap SHALL be monitored periodically
- **AND** low memory conditions SHALL be logged to USB serial
- **AND** system SHALL continue to function normally

