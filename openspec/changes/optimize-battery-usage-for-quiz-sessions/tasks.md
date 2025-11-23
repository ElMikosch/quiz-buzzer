# Implementation Tasks

## 1. Configuration and Constants
- [x] 1.1 Add power management constants to `src/config.h`
  - Add `#define CPU_FREQUENCY_MHZ 80`
  - Add `#define LIGHT_SLEEP_DURATION_READY_US 50000` (50ms)
  - Add `#define LIGHT_SLEEP_DURATION_LOCKED_US 100000` (100ms)
  - Add `#define LIGHT_SLEEP_DURATION_DISCONNECTED_US 200000` (200ms)
  - Add `#define MAX_BUTTON_RESPONSE_TIME_MS 50`
- [x] 1.2 Add power management includes to `src/buzzer_node.cpp`
  - Add `#include <esp_sleep.h>`
  - Add `#include <esp_pm.h>`

## 2. CPU Frequency Scaling
- [x] 2.1 Implement CPU frequency reduction in buzzer node initialization
  - Call `setCpuFrequencyMhz(CPU_FREQUENCY_MHZ)` in `setup()`
  - Verify frequency with `getCpuFrequencyMhz()` and log to Serial
  - Test that timing functions (millis, micros) remain accurate
- [ ] 2.2 Verify PWM/LEDC operation at 80MHz (HARDWARE TEST REQUIRED)
  - Ensure LED brightness control works correctly
  - Ensure blink timing remains accurate
  - Ensure fade effect remains smooth

## 3. WiFi Modem Sleep
- [x] 3.1 Enable WiFi modem sleep in buzzer node initialization
  - Call `esp_wifi_set_ps(WIFI_PS_MIN_MODEM)` after WiFi.mode(WIFI_STA)
  - Log modem sleep status to Serial
  - Verify ESP-NOW messages still transmit and receive correctly
- [ ] 3.2 Test ESP-NOW latency with modem sleep enabled (HARDWARE TEST REQUIRED)
  - Measure button press to transmission time
  - Verify latency stays under 50ms
  - Test message reception reliability during heartbeat

## 4. Light Sleep with GPIO Wake
- [x] 4.1 Configure GPIO wake source for button pin
  - Call `esp_sleep_enable_gpio_wakeup()` in setup()
  - Configure button pin (BUZZER_BUTTON_PIN) as wake source on LOW level
  - Add RTC_DATA_ATTR variables to preserve state across sleep
- [x] 4.2 Implement light sleep entry/exit logic
  - Create `enterLightSleep(uint64_t duration_us)` function
  - Configure timer wake as backup: `esp_sleep_enable_timer_wakeup(duration_us)`
  - Call `esp_light_sleep_start()` at end of loop()
  - Handle wake cause with `esp_sleep_get_wakeup_cause()`
- [x] 4.3 Track power mode state
  - Add `PowerMode` enum: ACTIVE, LOW_POWER, MINIMAL
  - Add `currentPowerMode` global variable
  - Implement `setPowerMode(PowerMode mode)` function to manage transitions

## 5. Power State Transitions
- [x] 5.1 Implement power mode transitions based on LED state changes
  - Modify `handleLEDCommand()` to call `setPowerMode()` when LED changes
  - LED_ON (ready) → ACTIVE mode (50ms sleep, GPIO wake enabled)
  - LED_OFF (locked) → LOW_POWER mode (100ms sleep, GPIO wake disabled)
  - LED_FADE (disconnected) → MINIMAL mode (200ms sleep, GPIO wake disabled)
- [x] 5.2 Implement dynamic sleep duration selection
  - Modify loop() to select sleep duration based on `currentPowerMode`
  - Pass appropriate duration to `enterLightSleep()`
  - Ensure wake timing matches requirements

## 6. Response Time Verification
- [ ] 6.1 Add latency measurement and logging
  - Record timestamp at button press detection (after wake)
  - Record timestamp at ESP-NOW transmission completion
  - Log total latency to Serial for testing
  - Add warning if latency exceeds 50ms threshold
- [ ] 6.2 Test end-to-end button response time
  - Test with oscilloscope or logic analyzer if available
  - Test button press to LED response time
  - Verify consistent timing across all 4 buzzer nodes
  - Test simultaneous button presses for fairness

## 7. Integration and Testing
- [ ] 7.1 Test power consumption measurement
  - Measure current draw in each power mode (multimeter or power profiler)
  - Document baseline vs optimized power consumption
  - Calculate expected battery life improvement
- [ ] 7.2 Test all game state transitions
  - Test ready → locked → ready cycle
  - Test disconnection and reconnection
  - Test partial lockout scenarios
  - Verify power modes transition correctly
- [ ] 7.3 Test quiz session scenarios
  - Run full quiz session simulation (30-60 minutes)
  - Verify battery life meets expectations
  - Verify no timing regressions or fairness issues
  - Test all 4 nodes simultaneously
- [ ] 7.4 Regression testing
  - Verify all existing LED patterns work correctly
  - Verify heartbeat and state sync still function
  - Verify serial command input still works
  - Run through all existing test scenarios

## 8. Documentation
- [x] 8.1 Update serial debug output
  - Log power mode transitions
  - Log CPU frequency at startup
  - Log WiFi modem sleep status
  - Log measured button response times
- [ ] 8.2 Add comments to power management code
  - Document sleep modes and wake sources
  - Explain power mode state machine
  - Document timing constraints and tradeoffs
