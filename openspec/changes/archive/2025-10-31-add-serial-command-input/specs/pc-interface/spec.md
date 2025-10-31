# pc-interface Spec Delta

## ADDED Requirements

### Requirement: Bidirectional Serial Communication
The serial interface SHALL support both inbound commands from PC and outbound event messages to PC.

#### Scenario: PC sends command to controller
- **WHEN** PC application writes a command to the serial port
- **THEN** the main controller SHALL receive the command
- **AND** parse it as a newline-terminated ASCII string
- **AND** execute the corresponding game action

#### Scenario: Command format specification
- **WHEN** sending commands from PC to controller
- **THEN** commands SHALL use the following format:
  - Correct answer: "CORRECT\n"
  - Wrong answer: "WRONG\n"
  - Full reset: "RESET\n"
- **AND** commands SHALL be case-sensitive
- **AND** commands SHALL be newline-terminated (\n or \r\n accepted)

### Requirement: Command Acknowledgment
The controller SHALL send acknowledgment messages when processing inbound commands.

#### Scenario: Command executed successfully
- **WHEN** a valid command is received and executed
- **THEN** the controller SHALL send the corresponding event message
- **AND** CORRECT command triggers "CORRECT\n" output
- **AND** WRONG command triggers "WRONG\n" output
- **AND** RESET command triggers "RESET\n" output

#### Scenario: Command ignored due to invalid state
- **WHEN** CORRECT or WRONG command is received
- **AND** no buzzer is currently selected
- **THEN** the controller SHALL NOT send any acknowledgment
- **AND** game state SHALL remain unchanged

### Requirement: Command Reliability
Serial commands SHALL be processed reliably without interfering with existing functionality.

#### Scenario: Commands work alongside physical buttons
- **WHEN** both serial commands and physical buttons are used
- **THEN** both input methods SHALL produce identical results
- **AND** trigger the same game state changes
- **AND** generate the same serial output messages

#### Scenario: Command processing latency
- **WHEN** a command is received via serial
- **THEN** it SHALL be processed within 10ms
- **AND** game state SHALL update immediately
- **AND** ESP-NOW LED commands SHALL be sent within 20ms

#### Scenario: Invalid command handling
- **WHEN** an invalid or malformed command is received
- **THEN** the controller SHALL ignore it
- **AND** continue normal operation
- **AND** NOT crash or hang
- **AND** optionally log a warning for debugging

### Requirement: PC Software Integration
PC applications SHALL be able to control game flow via serial commands.

#### Scenario: Quiz software sends correct answer
- **WHEN** quiz software determines answer is correct
- **AND** sends "CORRECT\n" to serial port
- **THEN** the controller SHALL reset game to ready state
- **AND** quiz software SHALL receive "CORRECT\n" confirmation
- **AND** all buzzers SHALL light up (via ESP-NOW)

#### Scenario: Quiz software sends wrong answer
- **WHEN** quiz software determines answer is wrong
- **AND** sends "WRONG\n" to serial port
- **THEN** the controller SHALL lock out current buzzer
- **AND** quiz software SHALL receive "WRONG\n" confirmation
- **AND** other buzzers SHALL remain active

#### Scenario: Quiz software resets game
- **WHEN** quiz software needs to reset the game
- **AND** sends "RESET\n" to serial port
- **THEN** the controller SHALL clear all game state
- **AND** quiz software SHALL receive "RESET\n" confirmation
- **AND** all buzzers SHALL return to ready state

#### Scenario: Automated testing
- **WHEN** running automated tests
- **THEN** test scripts SHALL be able to send commands via serial
- **AND** verify game behavior by monitoring serial output
- **AND** simulate game scenarios without physical button presses

## MODIFIED Requirements

None - this change extends existing serial communication without breaking compatibility.

## REMOVED Requirements

None
