# Tasks: update-led-behavior-patterns

## Implementation Tasks

- [x] **Task 1.1**: Add PWM/LEDC configuration constants to config.h
  - Add LED PWM channel number (0-15)
  - Add PWM frequency (5000 Hz recommended)
  - Add PWM resolution (8-bit = 0-255)
  - Add fade step increment and timing constants
  - Add fast-blink duration constant (3000ms)
  - Add fast-blink interval constant (100ms for 5Hz)

- [x] **Task 1.2**: Add new LED state enum and variables
  - Add LED_FADE state to LEDState enum in protocol.h
  - Add LED_BLINK_FAST sub-state for pressed buzzer
  - Add fade-related variables (brightness, fadeDirection, fadeStep)
  - Add blink-phase variables (fastBlinkStartTime, isInFastBlinkPhase)

- [x] **Task 1.3**: Initialize PWM/LEDC in setup()
  - Call ledcSetup() with channel, frequency, and resolution
  - Call ledcAttachPin() to attach LED pin to PWM channel
  - Replace digitalWrite() calls with ledcWrite() for PWM control
  - Test PWM initialization and verify LED responds to brightness changes

- [x] **Task 1.4**: Implement smooth breathing fade logic
  - Create handleLEDFade() function for smooth brightness transitions
  - Implement fade-in from 0 to 255 brightness
  - Implement fade-out from 255 to 0 brightness
  - Use smooth sine or linear interpolation for natural breathing effect
  - Call during disconnected state only

- [x] **Task 1.5**: Implement two-stage blink pattern for pressed state
  - Detect when transitioning to LED_BLINK state (pressed acknowledged)
  - Start 3-second fast-blink phase at 5Hz (100ms on/off)
  - Track elapsed time from fast-blink start
  - After 3 seconds, transition to normal 2Hz blink (500ms on/off)
  - Ensure smooth transition between phases

- [x] **Task 1.6**: Update handleLED() function
  - Add case for LED_FADE state (call handleLEDFade)
  - Update LED_BLINK case to support two-stage pattern
  - Keep LED_ON as full brightness PWM (255)
  - Keep LED_OFF as zero brightness PWM (0)
  - Remove old digitalWrite() calls, use ledcWrite() throughout

- [x] **Task 1.7**: Update connection state transitions
  - Set LED to LED_FADE when entering disconnected state
  - Set LED to LED_ON (solid) when connection established
  - Ensure fade stops and LED transitions to solid on reconnect
  - Test reconnection scenarios

## Testing Tasks

- [ ] **Task 2.1**: Test PWM fade implementation
  - Verify smooth breathing effect during disconnected state
  - Check fade timing and brightness curve smoothness
  - Ensure no visible flicker or stuttering
  - Test on actual hardware (PWM frequency matters)

- [ ] **Task 2.2**: Test two-stage blink pattern
  - Press buzzer and verify 5Hz fast blink starts immediately
  - Verify timing: exactly 3 seconds of fast blink
  - Verify smooth transition to 2Hz normal blink after 3 seconds
  - Test multiple button presses in sequence

- [ ] **Task 2.3**: Test state transitions
  - Disconnect controller and verify breathing fade starts
  - Reconnect and verify fade stops, LED goes solid ON
  - Press buzzer while connected and verify fast→slow blink
  - Reset game and verify LED returns to solid ON

- [ ] **Task 2.4**: Test edge cases
  - Power cycle during fast-blink phase (should resume correct state)
  - Disconnect during fast-blink phase (should switch to breathing)
  - Multiple rapid state changes
  - Verify locked-out state still shows LED OFF

## Documentation Tasks

- [x] **Task 3.1**: Update buzzer-node spec
  - Modify LED Visual Feedback requirement
  - Update disconnected state scenario to describe breathing effect
  - Add two-stage blink pattern scenario for pressed state
  - Document PWM usage and timing constants
  - Update timing specifications (5Hz fast, 2Hz slow, fade parameters)

- [x] **Task 3.2**: Update code comments
  - Document PWM channel configuration
  - Explain fade algorithm and brightness calculation
  - Document two-stage blink state machine
  - Add timing diagrams in comments if helpful
