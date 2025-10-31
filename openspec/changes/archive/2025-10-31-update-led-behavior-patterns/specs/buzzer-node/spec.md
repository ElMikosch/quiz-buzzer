# buzzer-node Spec Delta

## MODIFIED Requirements

### Requirement: LED Visual Feedback
Each buzzer node SHALL control an LED to indicate the current game state based on commands from the main controller, using PWM-based brightness control for smooth visual effects.

#### Scenario: Ready state - LED solid on
- **WHEN** the game is in ready state
- **AND** the node is connected to main controller
- **THEN** the LED SHALL be continuously lit at full brightness (PWM duty cycle 255)
- **AND** the button SHALL be active

#### Scenario: Disconnected state - LED breathing effect
- **WHEN** the node is disconnected from main controller
- **THEN** the LED SHALL display a smooth breathing effect (fade in and out)
- **AND** use PWM to gradually increase brightness from 0 to 255
- **AND** use PWM to gradually decrease brightness from 255 to 0
- **AND** create a continuous breathing pattern until connection is restored
- **AND** the button SHALL be inactive

#### Scenario: Selected buzzer - two-stage blink pattern
- **WHEN** this buzzer was the first to press
- **AND** the main controller has acknowledged the press
- **THEN** the LED SHALL blink rapidly at 5Hz (100ms on, 100ms off) for exactly 3 seconds
- **AND** after 3 seconds the LED SHALL transition to slow blink at 2Hz (500ms on, 500ms off)
- **AND** continue slow blinking until game state changes
- **AND** the button SHALL be inactive

#### Scenario: Locked out - LED off
- **WHEN** this buzzer gave a wrong answer
- **THEN** the LED SHALL be off (PWM duty cycle 0)
- **AND** the button SHALL be inactive

#### Scenario: Other buzzer selected - LED off
- **WHEN** a different buzzer was first to press
- **THEN** the LED SHALL be off (PWM duty cycle 0)
- **AND** the button SHALL be inactive

## ADDED Requirements

### Requirement: PWM LED Control
Each buzzer node SHALL use ESP32 LEDC (PWM) peripheral to control LED brightness for smooth visual effects.

#### Scenario: PWM initialization at startup
- **WHEN** the node powers on
- **THEN** it SHALL configure LEDC channel 0 with 5kHz frequency and 8-bit resolution
- **AND** attach the LED pin to the PWM channel
- **AND** verify PWM initialization succeeds before continuing

#### Scenario: Smooth breathing fade implementation
- **WHEN** in disconnected state with breathing effect active
- **THEN** the node SHALL update LED brightness in small increments (fade steps)
- **AND** create smooth transitions between brightness levels
- **AND** use timing control to achieve visible breathing rate (~2-3 seconds per full cycle)
- **AND** ensure no visible flicker or stuttering

#### Scenario: Fast-to-slow blink transition timing
- **WHEN** transitioning from fast blink to slow blink after 3 seconds
- **THEN** the node SHALL track elapsed time from fast-blink start
- **AND** switch blink interval from 100ms to 500ms exactly at 3-second mark
- **AND** maintain blink state continuity during transition (no glitches)

## MODIFIED Requirements (updates to existing scenarios)

### Requirement: Connection Monitoring

#### Scenario: Connection timeout detected (MODIFIED)
- **WHEN** no heartbeat received for CONNECTION_TIMEOUT_MS (5000ms)
- **THEN** the node SHALL enter disconnected state
- **AND** set LED to breathing fade effect (not rapid blink)
- **AND** disable button presses
- **AND** log disconnection event to Serial

### Requirement: Power-On Initialization

#### Scenario: Successful initialization (MODIFIED)
- **WHEN** the node powers on
- **THEN** it SHALL initialize GPIO pins (button input with pullup, LED output)
- **AND** reserve a GPIO pin for future speaker integration (not implemented)
- **AND** initialize PWM/LEDC for LED control
- **AND** set custom MAC address based on NODE_ID
- **AND** initialize ESP-NOW with main controller MAC address
- **AND** set LED to breathing fade (disconnected state until first heartbeat)
- **AND** print initialization status to Serial at 115200 baud
- **AND** include node ID and custom MAC address in output
