# main-controller Spec Delta

## MODIFIED Requirements

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
