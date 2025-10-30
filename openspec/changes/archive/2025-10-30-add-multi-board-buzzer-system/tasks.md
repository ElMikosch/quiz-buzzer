## 1. Project Structure Setup
- [x] 1.1 Create separate source files for buzzer node and main controller
- [x] 1.2 Add platformio environments for different board roles (buzzer_node_1-4, main_controller)
- [x] 1.3 Define common protocol header file for ESP-NOW messages
- [x] 1.4 Set up conditional compilation for buzzer vs controller roles
- [x] 1.5 Add build flags for NODE_ID (1-4) per buzzer environment
- [x] 1.6 Create config.h for pin assignments and constants (LED blink rate, custom MAC base)

## 2. Buzzer Node Implementation
- [x] 2.1 Configure GPIO pins (button input with pullup, LED output)
- [x] 2.2 Set custom MAC address based on NODE_ID using esp_wifi_set_mac()
- [x] 2.3 Implement button debouncing and press detection
- [x] 2.4 Set up ESP-NOW communication with main controller
- [x] 2.5 Implement button press message transmission with retry logic
- [x] 2.6 Implement LED control based on received commands (on/off/blink)
- [x] 2.7 Add serial output showing node ID and custom MAC address
- [x] 2.8 Reserve GPIO pin for future speaker integration (document in comments)
- [x] 2.9 Test individual buzzer node functionality

## 3. Main Controller - Communication Layer
- [x] 3.1 Initialize ESP-NOW as receiver for 4 buzzer nodes
- [x] 3.2 Hardcode custom MAC addresses of buzzer nodes (AA:BB:CC:DD:EE:01-04)
- [x] 3.3 Implement message reception handler for button presses
- [x] 3.4 Implement broadcast/unicast LED command transmission
- [x] 3.5 Add message acknowledgment and retry logic
- [x] 3.6 Test communication with all 4 nodes

## 4. Main Controller - Game State Machine
- [x] 4.1 Define game states (READY, LOCKED, PARTIAL_LOCKOUT)
- [x] 4.2 Track which buzzers are currently allowed to press
- [x] 4.3 Implement first-press detection and lockout logic
- [x] 4.4 Track currently selected buzzer (for blinking LED)
- [x] 4.5 Implement correct answer handler (reset to READY, all LEDs on)
- [x] 4.6 Implement wrong answer handler (PARTIAL_LOCKOUT, exclude wrong buzzer)
- [x] 4.7 Implement full reset handler (clear all state, return to READY)
- [x] 4.8 Add state transition validation and logging

## 5. Main Controller - Control Interface
- [x] 5.1 Configure 3 GPIO input pins for control buttons with pullups
- [x] 5.2 Implement button debouncing for control inputs
- [x] 5.3 Map button 1 to "correct answer" action
- [x] 5.4 Map button 2 to "wrong answer" action
- [x] 5.5 Map button 3 to "full reset" action
- [x] 5.6 Add visual/serial feedback for control button presses

## 6. PC Interface - USB Serial
- [x] 6.1 Configure serial communication at 115200 baud
- [x] 6.2 Implement text-based message protocol (BUZZER:<id>, CORRECT, WRONG, RESET)
- [x] 6.3 Send buzzer press messages to serial port
- [x] 6.4 Send control button messages to serial port
- [x] 6.5 Add message queuing for rapid events (max 10 messages)
- [x] 6.6 Implement newline-terminated message framing
- [x] 6.7 Test PC serial port reception and parsing

## 7. Integration and Testing
- [x] 7.1 Test full workflow: ready → buzzer press → lock → correct answer → reset
- [x] 7.2 Test wrong answer workflow: ready → press → wrong → press again → correct
- [x] 7.3 Test multiple wrong answers in sequence with selective lockout
- [x] 7.4 Test full reset from any state
- [x] 7.5 Verify LED feedback on all 4 nodes for each state
- [x] 7.6 Test ESP-NOW reliability and message timing
- [x] 7.7 Verify PC receives correct serial messages for all events
- [x] 7.8 Load test with rapid button presses

## 8. Documentation and Future Prep
- [x] 8.1 Document GPIO pin assignments for all boards
- [x] 8.2 Document ESP-NOW message protocol and serial message format
- [x] 8.3 Document game state machine transitions
- [x] 8.4 Document custom MAC address scheme (AA:BB:CC:DD:EE:01-04)
- [x] 8.5 Add comments marking GPIO pins reserved for future speaker integration
- [x] 8.6 Create flashing instructions for each board type with build environment names
- [x] 8.7 Add troubleshooting guide for common issues
- [x] 8.8 Document speaker feature as future enhancement (out of scope for v1)
