# Tasks: add-serial-command-input

## Implementation Tasks

- [x] **Task 1.1**: Add serial input buffer and parsing function in controller.cpp
  - Add string buffer to accumulate incoming characters until newline
  - Implement `handleSerialInput()` function to read Serial.available() and parse commands
  - Handle line-based protocol (newline-terminated commands)

- [x] **Task 1.2**: Implement command parsing logic
  - Parse "CORRECT\n" and call `handleCorrectAnswer()`
  - Parse "WRONG\n" and call `handleWrongAnswer()`
  - Parse "RESET\n" and call `handleFullReset()`
  - Ignore unknown commands with optional debug warning

- [x] **Task 1.3**: Integrate serial input handler into main loop
  - Call `handleSerialInput()` in `loop()` after `processMessageQueue()`
  - Ensure non-blocking operation (only process available bytes)
  - Test that serial input and button input work simultaneously

- [x] **Task 1.4**: Add command echo/acknowledgment (optional)
  - When command is received, optionally echo "CMD:<command>\n" for confirmation
  - Helps with debugging and PC software integration

## Testing Tasks

- [x] **Task 2.1**: Unit test command parsing
  - Test each command string (CORRECT, WRONG, RESET)
  - Test partial commands and buffer handling
  - Test commands with different line endings (\n, \r\n)

- [x] **Task 2.2**: Integration test with hardware
  - Send commands via serial terminal while game is running
  - Verify game state changes correctly (same as button presses)
  - Test concurrent button press and serial command
  - Verify serial output messages still work correctly

- [x] **Task 2.3**: Test invalid input handling
  - Send unknown commands and verify they're ignored
  - Send malformed input and verify no crashes
  - Test very long lines (buffer overflow protection)

## Documentation Tasks

- [x] **Task 3.1**: Update pc-interface spec
  - Add requirement for bidirectional serial communication
  - Document inbound command format (CORRECT\n, WRONG\n, RESET\n)
  - Add scenarios for command reception and handling

- [x] **Task 3.2**: Update main-controller spec
  - Add requirement for serial command processing
  - Document command parsing and execution
  - Add scenarios for command handling in different game states

- [x] **Task 3.3**: Update README
  - Add section on serial command interface
  - Provide examples of sending commands from PC
  - Document command format and expected behavior
