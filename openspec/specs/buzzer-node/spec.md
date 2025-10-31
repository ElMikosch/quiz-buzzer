# buzzer-node Specification

## Purpose
TBD - created by archiving change add-multi-board-buzzer-system. Update Purpose after archive.
## Requirements
### Requirement: Button Press Detection
Each buzzer node SHALL detect button press events with debouncing and transmit them to the main controller.

#### Scenario: Button press detected
- **WHEN** a player presses the buzzer button
- **THEN** the node SHALL debounce the input for 50ms
- **AND** transmit a button press message to the main controller
- **AND** include node ID (1-4) and timestamp in the message

#### Scenario: Button already pressed during lockout
- **WHEN** a player presses the buzzer button
- **AND** the node is in locked-out state (LED off)
- **THEN** the node SHALL ignore the button press
- **AND** NOT transmit any message

#### Scenario: Rapid repeated presses
- **WHEN** a player presses the buzzer multiple times rapidly
- **THEN** the node SHALL debounce each press
- **AND** transmit at most one message per 100ms

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

### Requirement: ESP-NOW Communication
Each buzzer node SHALL communicate with the main controller using ESP-NOW protocol.

#### Scenario: Successful message transmission
- **WHEN** transmitting a button press message
- **THEN** the node SHALL use ESP-NOW unicast to the main controller MAC address
- **AND** retry up to 3 times if transmission fails
- **AND** wait 10ms between retries

#### Scenario: Receiving LED command
- **WHEN** receiving an LED command from the main controller
- **THEN** the node SHALL validate the message format
- **AND** update the LED state (on/off/blink)
- **AND** apply the change within 50ms

#### Scenario: Communication timeout
- **WHEN** no message received from main controller for 5 seconds
- **THEN** the node SHALL enter error state
- **AND** blink LED rapidly (5Hz) to indicate communication error

### Requirement: Node Identification
Each buzzer node SHALL have a unique identifier (1-4) configured in firmware.

#### Scenario: Node ID in messages
- **WHEN** transmitting any message
- **THEN** the node SHALL include its configured ID (1-4)
- **AND** the ID SHALL be immutable at runtime

#### Scenario: Node ID from build configuration
- **WHEN** the firmware is compiled
- **THEN** the node ID SHALL be set via build flag (NODE_ID)
- **AND** the ID SHALL be validated to be in range 1-4

### Requirement: Custom MAC Address
Each buzzer node SHALL set a custom MAC address based on its node ID to simplify configuration.

#### Scenario: MAC address assignment
- **WHEN** the node powers on
- **THEN** it SHALL set its MAC address to {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, NODE_ID}
- **AND** use esp_wifi_set_mac() before ESP-NOW initialization
- **AND** print the custom MAC address to Serial

#### Scenario: MAC address verification
- **WHEN** ESP-NOW is initialized
- **THEN** the custom MAC address SHALL be active
- **AND** visible to the main controller
- **AND** match the expected pattern (ending in 0x01-0x04)

### Requirement: Power-On Initialization
Each buzzer node SHALL initialize hardware and communication on power-up.

#### Scenario: Successful initialization
- **WHEN** the node powers on
- **THEN** it SHALL initialize GPIO pins (button input with pullup, LED output)
- **AND** reserve a GPIO pin for future speaker integration (not implemented)
- **AND** initialize PWM/LEDC for LED control
- **AND** set custom MAC address based on NODE_ID
- **AND** initialize ESP-NOW with main controller MAC address
- **AND** set LED to breathing fade (disconnected state until first heartbeat)
- **AND** print initialization status to Serial at 115200 baud
- **AND** include node ID and custom MAC address in output

#### Scenario: Initialization failure
- **WHEN** ESP-NOW initialization fails
- **THEN** the node SHALL blink LED rapidly (10Hz)
- **AND** print error to Serial
- **AND** retry initialization every 5 seconds

### Requirement: Connection Monitoring
Each buzzer node SHALL monitor its connection to the main controller and detect disconnections.

#### Scenario: Normal heartbeat reception
- **WHEN** the node receives a heartbeat message from the main controller
- **THEN** it SHALL update its last-heartbeat timestamp
- **AND** maintain normal operation with current LED state

#### Scenario: Connection timeout detected
- **WHEN** no heartbeat received for CONNECTION_TIMEOUT_MS (5000ms)
- **THEN** the node SHALL enter disconnected state
- **AND** set LED to breathing fade effect (not rapid blink)
- **AND** disable button presses
- **AND** log disconnection event to Serial

#### Scenario: Heartbeat received after timeout
- **WHEN** in disconnected state
- **AND** a heartbeat message is received
- **THEN** the node SHALL detect reconnection
- **AND** send MSG_STATE_REQUEST to main controller
- **AND** wait for state sync before updating LED
- **AND** log reconnection event to Serial

### Requirement: State Synchronization
Each buzzer node SHALL request and receive current game state when reconnecting, interpreting the game state mode to determine correct LED behavior.

#### Scenario: Receive state sync message (MODIFIED)
- **WHEN** receiving MSG_STATE_SYNC from main controller
- **THEN** the node SHALL extract game state from the value field:
  - Bits 0-3: locked buzzers bitmask
  - Bits 4-6: selected buzzer ID (0 = none)
  - Bit 7: game state mode (0 = LOCKED/READY, 1 = PARTIAL_LOCKOUT)
- **AND** determine if this node is locked: `isLocked = lockedBuzzers & (1 << (NODE_ID - 1))`
- **AND** determine if this node is selected: `isSelected = (selectedBuzzer == NODE_ID)`
- **AND** determine game state mode: `isPartialLockout = (value & 0x80) != 0`

#### Scenario: State sync LED logic for locked mode (ADDED)
- **WHEN** receiving state sync
- **AND** bit 7 = 0 (locked/ready mode)
- **AND** selectedBuzzer != 0 (locked state)
- **THEN** if this node is selected: set LED to BLINK (two-stage pattern)
- **AND** if this node is not selected: set LED to OFF
- **AND** restore normal operation

#### Scenario: State sync LED logic for partial lockout mode (ADDED)
- **WHEN** receiving state sync
- **AND** bit 7 = 1 (partial lockout mode)
- **THEN** if this node is selected: set LED to BLINK (two-stage pattern)
- **AND** if this node is in lockedBuzzers bitmask: set LED to OFF
- **AND** if this node is neither selected nor locked: set LED to ON
- **AND** restore normal operation

#### Scenario: State sync LED logic for ready state (ADDED)
- **WHEN** receiving state sync
- **AND** bit 7 = 0 (locked/ready mode)
- **AND** selectedBuzzer == 0 (ready state)
- **THEN** set LED to ON for all nodes
- **AND** restore normal operation

### Requirement: Power Cycle Recovery
Each buzzer node SHALL automatically recover correct state after power cycle.

#### Scenario: Power cycle during ready state
- **WHEN** node powers on
- **AND** main controller is in ready state
- **THEN** the node SHALL connect to main controller
- **AND** receive heartbeat within 5 seconds
- **AND** request state sync
- **AND** restore LED to on (ready state)

#### Scenario: Power cycle during locked state
- **WHEN** node powers on
- **AND** game is in locked state
- **THEN** the node SHALL request and receive current state
- **AND** restore correct LED (off if not selected, two-stage blink if selected)
- **AND** maintain proper button disable state

#### Scenario: Power cycle during partial lockout
- **WHEN** node powers on
- **AND** game is in partial lockout state
- **AND** this node is locked out
- **THEN** the node SHALL restore LED to off
- **AND** disable button presses

