# main-controller Spec Delta

## ADDED Requirements

### Requirement: Serial Command Input Processing
The main controller SHALL accept game control commands via USB serial port.

#### Scenario: Receive CORRECT command from serial
- **WHEN** the PC sends "CORRECT\n" via serial port
- **AND** a buzzer is currently selected
- **THEN** the controller SHALL execute correct answer logic
- **AND** reset game state to ready
- **AND** turn on all buzzer LEDs
- **AND** send "CORRECT\n" message back to PC

#### Scenario: Receive WRONG command from serial
- **WHEN** the PC sends "WRONG\n" via serial port
- **AND** a buzzer is currently selected
- **THEN** the controller SHALL execute wrong answer logic
- **AND** lock out the currently selected buzzer
- **AND** enter partial lockout state
- **AND** send "WRONG\n" message back to PC

#### Scenario: Receive RESET command from serial
- **WHEN** the PC sends "RESET\n" via serial port
- **THEN** the controller SHALL execute full reset logic
- **AND** clear all game state
- **AND** turn on all buzzer LEDs
- **AND** return to ready state
- **AND** send "RESET\n" message back to PC

#### Scenario: Serial command in invalid state
- **WHEN** CORRECT or WRONG command is received
- **AND** no buzzer is currently selected
- **THEN** the controller SHALL ignore the command
- **AND** NOT change game state
- **AND** optionally log warning to serial debug output

#### Scenario: Unknown command received
- **WHEN** an unrecognized command is received via serial
- **THEN** the controller SHALL ignore the command
- **AND** NOT change game state
- **AND** optionally log warning to serial debug output

#### Scenario: Serial commands work alongside buttons
- **WHEN** both physical buttons and serial commands are active
- **THEN** both input methods SHALL trigger the same game logic
- **AND** function identically (correct/wrong/reset behavior)
- **AND** NOT interfere with each other

### Requirement: Non-Blocking Serial Input
Serial command processing SHALL not block the main loop or interfere with ESP-NOW communication.

#### Scenario: Serial input availability check
- **WHEN** processing serial input each loop iteration
- **THEN** the controller SHALL check Serial.available()
- **AND** only process available bytes (non-blocking)
- **AND** NOT wait for input if none is available

#### Scenario: Line-based command buffering
- **WHEN** serial bytes are received
- **THEN** the controller SHALL accumulate bytes in a buffer
- **AND** wait for newline character ('\\n') before parsing
- **AND** parse complete lines as commands
- **AND** clear buffer after processing each command

#### Scenario: Buffer overflow protection
- **WHEN** serial input exceeds buffer size (256 bytes)
- **THEN** the controller SHALL discard the buffer
- **AND** wait for next newline to resynchronize
- **AND** NOT crash or hang
- **AND** log buffer overflow warning

#### Scenario: Concurrent ESP-NOW and serial handling
- **WHEN** processing serial commands
- **THEN** ESP-NOW message handling SHALL continue normally
- **AND** buzzer presses SHALL be processed immediately
- **AND** heartbeat broadcasting SHALL continue
- **AND** connection monitoring SHALL continue
- **AND** serial input SHALL NOT add significant latency (<5ms)

## MODIFIED Requirements

None - this change adds new functionality without modifying existing behavior.

## REMOVED Requirements

None
