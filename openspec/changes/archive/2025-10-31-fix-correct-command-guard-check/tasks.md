# Tasks

## Implementation
- [x] Add selectedBuzzer == 0 guard check at start of handleCorrectAnswer() function in src/controller.cpp
- [x] Add Serial.println message "No buzzer selected, ignoring CORRECT command" when guard check triggers
- [x] Add early return when guard check triggers to prevent state changes
- [x] Ensure message format matches existing handleWrongAnswer() guard message

## Validation
- [x] Manual test: Send CORRECT command via serial when no buzzer is selected - verify ignored with message
- [x] Manual test: Send WRONG command via serial when no buzzer is selected - verify ignored with message
- [x] Manual test: Press physical CORRECT button when no buzzer selected - verify ignored
- [x] Manual test: Normal flow - press buzzer, send CORRECT command - verify works correctly
- [x] Verify no CORRECT message sent to PC interface when command is ignored
- [x] Run openspec validate to ensure spec compliance

## Documentation
- [ ] Update main-controller spec via openspec archive process (automatic)
