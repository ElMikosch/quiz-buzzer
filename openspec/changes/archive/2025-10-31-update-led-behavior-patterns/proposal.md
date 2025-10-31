# Change Proposal: update-led-behavior-patterns

## Why

Current LED behavior uses simple on/off blinking patterns that don't provide smooth visual feedback. The disconnection state uses harsh fast blinking, and the pressed/selected state immediately transitions to slow blinking without an initial attention-grabbing phase. Improving LED patterns with smooth breathing effects and staged blink speeds will provide better visual feedback and user experience.

## What Changes

- Replace disconnected state fast blinking with smooth PWM-based breathing effect (fade in/out)
- Add two-stage blink pattern for pressed/selected state: 5Hz fast blink for 3 seconds, then transition to normal 2Hz blink
- Implement ESP32 LEDC (PWM) peripheral for smooth brightness control during fade
- Keep ready state as solid ON (no fade when connected)
- Keep locked-out state as OFF (unchanged)

## Impact

- **Affected specs**: buzzer-node
- **Affected code**:
  - `src/buzzer_node.cpp` - Update LED handling logic, add PWM fade implementation, add two-stage blink state machine
  - `src/config.h` - Add new timing constants for fade and fast-blink durations
- **Benefits**:
  - Smoother, more professional visual feedback during disconnection
  - More noticeable initial feedback when buzzer is pressed (fast blink gets attention)
  - Better user experience with breathing effect vs harsh blinking
  - No hardware changes required (uses ESP32 built-in PWM)
- **Risks**: 
  - Low - purely visual changes
  - PWM frequency must be set correctly to avoid LED flicker
  - Timing precision for 3-second fast-blink phase
