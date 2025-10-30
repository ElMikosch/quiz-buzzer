# main-controller Specification

## Purpose
TBD - created by archiving change add-multi-board-buzzer-system. Update Purpose after archive.
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

