# Change Proposal: fix-correct-command-guard-check

## Why

The `handleWrongAnswer()` function correctly checks if a buzzer is selected before processing the command (displaying "No buzzer selected, ignoring WRONG command" when appropriate). However, `handleCorrectAnswer()` lacks this guard check and will incorrectly reset the game state even when no buzzer is selected. This creates inconsistent behavior between CORRECT and WRONG commands, both from physical buttons and serial commands.

According to the main-controller spec (Requirement: Control Button Interface, Scenario: Control button pressed in invalid state), both correct and wrong buttons SHALL be ignored when no buzzer is currently selected. The serial command processing requirement states that serial commands should execute "identical behavior to physical button press", so this guard check applies to both input methods.

## What Changes

- Add a guard check in `handleCorrectAnswer()` to verify that `selectedBuzzer != 0` before processing
- Display "No buzzer selected, ignoring CORRECT command" message (matching WRONG command behavior)
- Return early without changing game state when no buzzer is selected
- Update main-controller spec to explicitly document this validation requirement for serial commands

## Impact

- **Affected specs**: main-controller
- **Affected code**:
  - `src/controller.cpp` - Add guard check to `handleCorrectAnswer()` function
- **Benefits**:
  - Consistent behavior between CORRECT and WRONG commands
  - Prevents unintended game state resets when no buzzer is active
  - Clearer user feedback via serial messages
  - Aligns implementation with existing spec requirements
- **Risks**: 
  - Very low - purely defensive programming
  - No behavior change for valid use cases (only affects invalid state)
  - PC interface or users may need to handle the new ignore case (though they should already handle it for WRONG)
