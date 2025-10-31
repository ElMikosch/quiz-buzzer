# pc-interface Specification

## Purpose
Defines the bidirectional USB serial communication protocol between the main controller and PC, including outbound game state messages and inbound control commands.
## Requirements
### Requirement: USB Serial Communication
The main controller SHALL communicate with the PC via USB serial port using a text-based protocol.

#### Scenario: Serial port initialization
- **WHEN** the main controller powers on
- **THEN** the USB serial port SHALL initialize at 115200 baud
- **AND** use 8 data bits, no parity, 1 stop bit (8N1)
- **AND** be ready to send messages within 2 seconds

#### Scenario: PC recognizes serial device
- **WHEN** the main controller is connected via USB
- **THEN** the PC SHALL recognize it as a USB CDC serial device
- **AND** create a COM port (Windows) or /dev/ttyUSB or /dev/ttyACM device (Linux/Mac)
- **AND** no driver installation SHALL be required

### Requirement: Message Protocol Format
Messages SHALL be sent as newline-terminated ASCII text strings.

#### Scenario: Message framing
- **WHEN** any message is sent to the PC
- **THEN** it SHALL be ASCII text
- **AND** terminated with newline character ('\n')
- **AND** be human-readable for debugging

#### Scenario: Message types
- **WHEN** sending messages
- **THEN** the following message formats SHALL be used:
  - Buzzer press: "BUZZER:<id>\n" where <id> is 1-4
  - Correct answer: "CORRECT\n"
  - Wrong answer: "WRONG\n"
  - Reset: "RESET\n"

### Requirement: Buzzer Press Messages
When a buzzer is pressed and registered, the main controller SHALL send a buzzer message to the PC.

#### Scenario: Buzzer 1 pressed
- **WHEN** buzzer node 1 button is pressed and registered
- **THEN** the PC SHALL receive "BUZZER:1\n" via serial port

#### Scenario: Buzzer 2 pressed
- **WHEN** buzzer node 2 button is pressed and registered
- **THEN** the PC SHALL receive "BUZZER:2\n" via serial port

#### Scenario: Buzzer 3 pressed
- **WHEN** buzzer node 3 button is pressed and registered
- **THEN** the PC SHALL receive "BUZZER:3\n" via serial port

#### Scenario: Buzzer 4 pressed
- **WHEN** buzzer node 4 button is pressed and registered
- **THEN** the PC SHALL receive "BUZZER:4\n" via serial port

#### Scenario: Message timing
- **WHEN** a buzzer press is registered
- **THEN** the message SHALL be sent within 50ms
- **AND** arrive at PC within 100ms of button press

### Requirement: Control Button Messages
The three control buttons SHALL send corresponding messages to the PC.

#### Scenario: Correct answer button
- **WHEN** the correct answer button is pressed
- **AND** a buzzer is currently selected
- **THEN** the PC SHALL receive "CORRECT\n" via serial port

#### Scenario: Wrong answer button
- **WHEN** the wrong answer button is pressed
- **AND** a buzzer is currently selected
- **THEN** the PC SHALL receive "WRONG\n" via serial port

#### Scenario: Reset button
- **WHEN** the reset button is pressed
- **THEN** the PC SHALL receive "RESET\n" via serial port

### Requirement: Message Queuing
The system SHALL queue messages when multiple events occur rapidly.

#### Scenario: Normal message sending
- **WHEN** a single message is generated
- **AND** the serial port is not busy
- **THEN** the message SHALL be sent immediately

#### Scenario: Rapid multiple events
- **WHEN** multiple messages are generated in rapid succession
- **THEN** messages SHALL be queued in FIFO order
- **AND** the queue SHALL hold up to 10 messages
- **AND** messages SHALL be sent as soon as possible

#### Scenario: Queue overflow
- **WHEN** more than 10 messages are queued
- **THEN** the oldest message SHALL be discarded
- **AND** a debug message SHALL be logged to serial
- **AND** newer messages SHALL be preserved

### Requirement: Serial Connection Handling
The system SHALL handle serial connection and disconnection gracefully.

#### Scenario: USB connected at power-on
- **WHEN** the main controller powers on with USB connected
- **THEN** serial communication SHALL initialize within 2 seconds
- **AND** be ready to send messages

#### Scenario: USB disconnected during operation
- **WHEN** USB cable is disconnected
- **THEN** the game state SHALL continue to function normally
- **AND** messages SHALL be queued (up to 10 messages)
- **AND** ESP-NOW communication SHALL continue
- **AND** LED control SHALL continue to work

#### Scenario: USB reconnected
- **WHEN** USB cable is reconnected after disconnection
- **THEN** serial communication SHALL reinitialize within 2 seconds
- **AND** queued messages SHALL be sent in order
- **AND** new messages SHALL be sent immediately

### Requirement: PC Software Integration
The system SHALL provide serial messages compatible with quiz software or custom applications.

#### Scenario: Quiz software receives buzzer press
- **WHEN** a buzzer is pressed
- **AND** quiz software is reading the serial port
- **THEN** the software SHALL receive "BUZZER:<id>\n"
- **AND** be able to determine which player buzzed in
- **AND** the message SHALL arrive within 100ms of button press

#### Scenario: Quiz software receives control commands
- **WHEN** a control button is pressed (correct/wrong/reset)
- **AND** quiz software is reading the serial port
- **THEN** the software SHALL receive the corresponding message
- **AND** be able to trigger corresponding actions

#### Scenario: Message parsing
- **WHEN** the PC receives messages
- **THEN** each message SHALL be on a separate line
- **AND** be parseable by splitting on newline
- **AND** be extractable using simple string matching

### Requirement: Inbound Serial Commands
The main controller SHALL accept control commands from the PC via serial port to trigger game state changes.

#### Scenario: CORRECT command received
- **WHEN** the PC sends "CORRECT\n" via serial port
- **THEN** the main controller SHALL execute the correct answer action
- **AND** respond with "CMD_ACK:CORRECT\n"
- **AND** transition game state identically to pressing the physical CORRECT button
- **AND** process the command within 10ms of receipt

#### Scenario: WRONG command received
- **WHEN** the PC sends "WRONG\n" via serial port
- **THEN** the main controller SHALL execute the wrong answer action
- **AND** respond with "CMD_ACK:WRONG\n"
- **AND** transition game state identically to pressing the physical WRONG button
- **AND** process the command within 10ms of receipt

#### Scenario: RESET command received
- **WHEN** the PC sends "RESET\n" via serial port
- **THEN** the main controller SHALL execute the full reset action
- **AND** respond with "CMD_ACK:RESET\n"
- **AND** transition game state identically to pressing the physical RESET button
- **AND** process the command within 10ms of receipt

#### Scenario: Unknown command received
- **WHEN** the PC sends an unrecognized command
- **THEN** the main controller SHALL ignore the command
- **AND** respond with "CMD_ERR:UNKNOWN:<command>\n"
- **AND** NOT change game state
- **AND** continue normal operation

#### Scenario: Command with different line endings
- **WHEN** the PC sends a command terminated with '\n', '\r', or '\r\n'
- **THEN** the main controller SHALL accept all variations
- **AND** parse the command correctly
- **AND** execute the appropriate action

#### Scenario: Buffer overflow protection
- **WHEN** the PC sends data exceeding 256 bytes without a newline
- **THEN** the main controller SHALL discard the buffer
- **AND** send "CMD_ERR:BUFFER_OVERFLOW\n"
- **AND** continue normal operation without crashing

#### Scenario: Concurrent serial and button input
- **WHEN** a serial command arrives while button inputs are active
- **THEN** both input sources SHALL be processed correctly
- **AND** neither SHALL be blocked
- **AND** commands SHALL execute in the order received
- **AND** the system SHALL remain responsive to both input methods

#### Scenario: Serial command during different game states
- **WHEN** serial commands are received in READY, LOCKED, or PARTIAL_LOCKOUT state
- **THEN** commands SHALL execute with identical behavior to physical buttons
- **AND** respect game state rules (e.g., WRONG requires selected buzzer)
- **AND** produce the same LED patterns and state transitions

### Requirement: Diagnostic and Debug Output
The system SHALL provide debug information via the same serial port without interfering with protocol messages.

#### Scenario: Debug message format
- **WHEN** debug information needs to be output
- **THEN** debug messages SHALL be prefixed with "DEBUG: "
- **AND** be distinguishable from protocol messages
- **AND** allow quiz software to filter them out

#### Scenario: Initialization messages
- **WHEN** the system initializes
- **THEN** initialization status SHALL be output to serial
- **AND** include ESP-NOW status, GPIO configuration, and MAC addresses
- **AND** be prefixed with "DEBUG: " for filtering

#### Scenario: Error messages
- **WHEN** an error occurs (communication timeout, invalid state)
- **THEN** error details SHALL be output to serial
- **AND** be prefixed with "DEBUG: ERROR: "
- **AND** include timestamp and error description

### Requirement: Performance and Reliability
Serial communication SHALL be reliable and low-latency.

#### Scenario: Message latency
- **WHEN** any message is sent
- **THEN** it SHALL appear on the serial port within 10ms
- **AND** be received by PC software within 50ms (including USB latency)

#### Scenario: No message loss
- **WHEN** messages are sent at normal rate (<10/second)
- **THEN** all messages SHALL be successfully transmitted
- **AND** none SHALL be lost or corrupted

#### Scenario: Buffer overflow prevention
- **WHEN** the serial buffer fills up
- **THEN** the system SHALL not block
- **AND** use message queuing to handle backpressure
- **AND** discard oldest messages if necessary

