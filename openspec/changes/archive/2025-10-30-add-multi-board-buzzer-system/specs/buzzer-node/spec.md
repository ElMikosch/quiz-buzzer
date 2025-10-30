## ADDED Requirements

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
Each buzzer node SHALL control an LED to indicate the current game state based on commands from the main controller.

#### Scenario: Ready state - LED solid on
- **WHEN** the game is in ready state
- **THEN** the LED SHALL be continuously lit
- **AND** the button SHALL be active

#### Scenario: Selected buzzer - LED blinking
- **WHEN** this buzzer was the first to press
- **THEN** the LED SHALL blink at 2Hz (500ms on, 500ms off)
- **AND** the button SHALL be inactive

#### Scenario: Locked out - LED off
- **WHEN** this buzzer gave a wrong answer
- **THEN** the LED SHALL be off
- **AND** the button SHALL be inactive

#### Scenario: Other buzzer selected - LED off
- **WHEN** a different buzzer was first to press
- **THEN** the LED SHALL be off
- **AND** the button SHALL be inactive

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
- **AND** set custom MAC address based on NODE_ID
- **AND** initialize ESP-NOW with main controller MAC address
- **AND** set LED to on (ready state)
- **AND** print initialization status to Serial at 115200 baud
- **AND** include node ID and custom MAC address in output

#### Scenario: Initialization failure
- **WHEN** ESP-NOW initialization fails
- **THEN** the node SHALL blink LED rapidly (10Hz)
- **AND** print error to Serial
- **AND** retry initialization every 5 seconds
