## ADDED Requirements

### Requirement: Power Mode Management
Each buzzer node SHALL implement dynamic power modes to optimize battery life while maintaining quiz fairness and response time guarantees.

#### Scenario: Active mode during ready state
- **WHEN** the node is connected and in ready state (LED solid on)
- **THEN** the node SHALL operate in active power mode
- **AND** maintain CPU frequency at 80MHz (reduced from default 240MHz)
- **AND** enable WiFi modem sleep between transmissions
- **AND** use light sleep with GPIO interrupt wake on button press
- **AND** guarantee button press detection within 50ms

#### Scenario: Low power mode during locked state
- **WHEN** the node is locked out (LED off, button inactive)
- **THEN** the node SHALL enter low power mode
- **AND** reduce CPU frequency to 80MHz
- **AND** enable WiFi modem sleep between heartbeat receptions
- **AND** disable button interrupt (button inactive)
- **AND** wake every 100ms to check for state change messages

#### Scenario: Minimal power mode during disconnected state
- **WHEN** the node is disconnected (LED breathing fade)
- **THEN** the node SHALL enter minimal power mode
- **AND** reduce CPU frequency to 80MHz
- **AND** enable WiFi modem sleep
- **AND** wake every 200ms for LED fade updates and heartbeat checking

### Requirement: Light Sleep with GPIO Interrupt
Each buzzer node SHALL use ESP32 light sleep mode with GPIO interrupt wake to reduce power consumption while maintaining rapid button response.

#### Scenario: Enter light sleep in ready state
- **WHEN** loop iteration completes
- **AND** node is in ready state with button active
- **THEN** the node SHALL configure GPIO wake on button press (LOW level)
- **AND** enter light sleep mode using esp_light_sleep_start()
- **AND** wake immediately on button press via GPIO interrupt
- **AND** wake on timer after maximum 50ms if no button press

#### Scenario: Button press wakes from light sleep
- **WHEN** in light sleep mode
- **AND** player presses the buzzer button
- **THEN** the GPIO interrupt SHALL wake the ESP32 within 5ms
- **AND** button press detection SHALL occur within 50ms total (including wake time)
- **AND** maintain debouncing logic after wake

#### Scenario: Timer wake from light sleep
- **WHEN** in light sleep mode
- **AND** no button press occurs
- **THEN** the node SHALL wake via timer after 50ms
- **AND** process LED updates, heartbeat checks, and ESP-NOW messages
- **AND** return to light sleep if button remains inactive

### Requirement: CPU Frequency Scaling
Each buzzer node SHALL reduce CPU frequency from default 240MHz to 80MHz to reduce power consumption while maintaining adequate performance for quiz operations.

#### Scenario: Set reduced CPU frequency at startup
- **WHEN** the node powers on
- **THEN** it SHALL call setCpuFrequencyMhz(80) during initialization
- **AND** verify frequency was set successfully
- **AND** adjust timing-sensitive operations if necessary
- **AND** log CPU frequency to Serial

#### Scenario: Maintain timing accuracy at reduced frequency
- **WHEN** operating at 80MHz CPU frequency
- **THEN** all timing operations (millis(), micros(), delays) SHALL remain accurate
- **AND** PWM/LEDC LED control SHALL maintain correct frequencies
- **AND** ESP-NOW communication SHALL function without timing issues
- **AND** button debouncing SHALL work correctly

### Requirement: WiFi Modem Sleep
Each buzzer node SHALL enable WiFi modem sleep to reduce power consumption between ESP-NOW message transmissions and receptions.

#### Scenario: Enable WiFi modem sleep during initialization
- **WHEN** WiFi and ESP-NOW are initialized
- **THEN** the node SHALL call esp_wifi_set_ps(WIFI_PS_MIN_MODEM)
- **AND** WiFi modem SHALL sleep between packets
- **AND** wake automatically for ESP-NOW transmissions
- **AND** wake automatically for ESP-NOW receptions

#### Scenario: Automatic wake for message transmission
- **WHEN** the node needs to send a button press message
- **THEN** WiFi modem SHALL wake automatically
- **AND** transmit the message via ESP-NOW
- **AND** return to modem sleep after transmission completes
- **AND** add no more than 10ms latency to transmission

#### Scenario: Automatic wake for message reception
- **WHEN** the main controller sends a message to the node
- **THEN** WiFi modem SHALL wake to receive the message
- **AND** process the message (LED command, heartbeat, state sync)
- **AND** return to modem sleep after processing
- **AND** maintain reliable message reception

### Requirement: Response Time Guarantee
Each buzzer node SHALL guarantee button press detection and transmission within 50ms to maintain quiz fairness, even with power optimization enabled.

#### Scenario: End-to-end button press latency measurement
- **WHEN** a player presses the buzzer button
- **THEN** the total time from physical press to ESP-NOW transmission SHALL be ≤50ms
- **AND** includes: GPIO wake time (≤5ms) + debounce (50ms max) + processing + transmission
- **AND** this latency SHALL be consistent across all power modes
- **AND** this latency SHALL be equal across all buzzer nodes (±5ms)

#### Scenario: Verify fairness across all nodes
- **WHEN** multiple players press buttons simultaneously
- **THEN** all nodes SHALL have equal wake and processing latency
- **AND** first physical press SHALL always be detected first
- **AND** timing variance between nodes SHALL be ≤5ms
- **AND** power optimization SHALL NOT create unfair timing advantages

### Requirement: Power State Transitions
Each buzzer node SHALL manage transitions between power modes based on game state changes to optimize battery life throughout the quiz session.

#### Scenario: Transition from disconnected to ready
- **WHEN** receiving first heartbeat after disconnection
- **AND** state sync indicates ready state
- **THEN** the node SHALL transition from minimal power mode to active mode
- **AND** enable GPIO wake interrupt for button press
- **AND** reduce sleep duration from 200ms to 50ms
- **AND** complete transition within 100ms

#### Scenario: Transition from ready to locked
- **WHEN** receiving LED command to turn off (locked out)
- **THEN** the node SHALL transition from active mode to low power mode
- **AND** disable GPIO wake interrupt (button inactive)
- **AND** increase sleep duration from 50ms to 100ms
- **AND** maintain heartbeat reception capability

#### Scenario: Transition from locked to ready
- **WHEN** receiving LED command to turn on (reset to ready)
- **THEN** the node SHALL transition from low power mode to active mode
- **AND** re-enable GPIO wake interrupt for button press
- **AND** reduce sleep duration from 100ms to 50ms
- **AND** restore full button responsiveness within 50ms
