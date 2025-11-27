# main-controller Specification Deltas

## ADDED Requirements

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
