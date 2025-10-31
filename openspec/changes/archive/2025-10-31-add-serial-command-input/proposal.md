# Change Proposal: add-serial-command-input

## Why

Currently, the main controller can only be controlled via physical buttons (CORRECT, WRONG, RESET). This requires manual button presses for game control. Adding serial command input enables remote control from PC software, automation, testing, and integration with quiz applications that need to trigger these actions programmatically.

## What Changes

- Add serial input parsing in main controller to accept command strings
- Implement command handlers for CORRECT, WRONG, and RESET commands
- Reuse existing game logic functions (handleCorrectAnswer, handleWrongAnswer, handleFullReset)
- Process serial commands in the main loop alongside button handling
- Maintain backwards compatibility with physical buttons (both inputs work simultaneously)

## Impact

- **Affected specs**: main-controller, pc-interface
- **Affected code**:
  - `src/controller.cpp` - Add serial input reading and command parsing
  - No protocol changes (serial is already unidirectional outbound, now becomes bidirectional)
- **Benefits**:
  - Enables PC software to control game flow
  - Simplifies testing and automation
  - Allows quiz applications to trigger correct/wrong/reset actions
  - No hardware changes required
- **Risks**: Low - reuses existing game logic, adds new input path only
