## Context
The quiz buzzer system requires 5 ESP32 boards working together: 4 buzzer nodes (player stations) and 1 main controller (game master). The system must handle real-time button press detection with sub-100ms response time, maintain consistent game state across distributed nodes, and provide a reliable interface to a PC for quiz software integration.

**Constraints:**
- Limited ESP32 memory (320KB RAM per board)
- Wireless communication latency (ESP-NOW typically 10-30ms)
- Real-time requirements for button response
- Serial communication at 115200 baud
- Battery power for buzzer nodes (1100mAh 3.7V LiPo, ~4 hours operation)
- Power consumption optimization required for battery life

**Stakeholders:**
- Quiz game host (uses control buttons)
- Players (use buzzers)
- Quiz software on PC (receives serial messages)

## Goals / Non-Goals

**Goals:**
- Reliable first-press detection across 4 distributed nodes
- Visual feedback via LEDs showing game state
- Simple 3-button control interface for game master
- PC integration via USB serial communication
- Support for answer validation workflow with selective lockout
- Future-ready for speaker audio feedback

**Non-Goals:**
- Network configuration UI (use hardcoded MAC addresses)
- Score tracking (handled by PC software)
- Display screen integration
- Mobile app control
- Over-the-air (OTA) firmware updates

## Decisions

### Decision 1: ESP-NOW vs WiFi/Bluetooth
**Choice:** ESP-NOW
**Rationale:**
- Lower latency (10-30ms vs 50-100ms for WiFi/BLE)
- No router/pairing required
- Lower power consumption
- Simpler protocol for point-to-multipoint
- Built-in for ESP32, no additional libraries

**Alternatives considered:**
- WiFi (MQTT): Requires router, higher latency, more complex
- Bluetooth Classic: Pairing complexity, limited connections
- Bluetooth BLE: Lower throughput, more complex state management
- Wired (I2C/SPI): Defeats purpose of distributed nodes

### Decision 2: Centralized vs Distributed State
**Choice:** Centralized state on main controller
**Rationale:**
- Single source of truth prevents race conditions
- Simpler debugging and state management
- Main controller has direct PC connection for output
- Buzzer nodes become thin clients (simpler firmware)

**Alternatives considered:**
- Distributed consensus: Too complex for embedded systems, unnecessary latency
- Peer-to-peer: Race conditions on simultaneous presses

### Decision 3: LED Control - Local vs Remote
**Choice:** Remote control from main controller
**Rationale:**
- Main controller knows authoritative game state
- Ensures LED state matches actual game state
- Allows synchronized visual effects (all on/off together)
- Simplifies buzzer node logic

**Trade-off:** Adds ~20-30ms latency for LED response, acceptable for visual feedback

### Decision 4: Serial Protocol vs USB HID Keyboard
**Choice:** USB Serial (CDC) Protocol
**Rationale:**
- Lolin32 Lite does not support native USB HID (uses USB-to-serial chip)
- Simple text-based protocol easy to parse
- Standard Serial.println() for implementation
- Quiz software can read COM/tty port
- Human-readable messages for debugging
- No special drivers needed (standard CDC-ACM)

**Protocol Format:**
```
BUZZER:<id>           // Buzzer 1-4 pressed
CORRECT               // Correct answer button
WRONG                 // Wrong answer button  
RESET                 // Reset button
```

**Alternatives considered:**
- USB HID Keyboard: Not supported by Lolin32 Lite hardware
- Binary serial protocol: Less readable, minimal performance gain
- Network socket: Requires WiFi infrastructure

### Decision 5: Message Protocol Design
**Choice:** Simple struct-based binary protocol
```cpp
struct BuzzerMessage {
  uint8_t node_id;      // 1-4
  uint8_t msg_type;     // BUTTON_PRESS, LED_CMD
  uint8_t value;        // LED state or press count
  uint32_t timestamp;   // millis() for deduplication
};
```
**Rationale:**
- Fixed size for predictable performance
- No serialization overhead (just memcpy)
- Timestamp for deduplication of retransmissions
- Simple to debug with Serial output

### Decision 6: Button Debouncing Strategy
**Choice:** Software debouncing with 50ms timeout
**Rationale:**
- No additional hardware (capacitors)
- Configurable per button if needed
- Standard approach for embedded systems
- 50ms sufficient for mechanical switches

### Decision 7: LED Blink Implementation
**Choice:** Non-blocking timer-based blink on buzzer nodes
**Rationale:**
- Keeps main loop responsive for button presses
- Simple state machine (on/off/blink)
- Blink rate controlled by local timer, reduces message traffic

### Decision 8: Project Structure
**Choice:** Single repository with conditional compilation
```
src/
  main.cpp          // Entry point with #ifdef for role selection
  buzzer_node.cpp   // Buzzer node implementation
  controller.cpp    // Main controller implementation
  protocol.h        // Shared message definitions
  config.h          // Pin assignments and constants
```
**Rationale:**
- Shared protocol definitions prevent version mismatch
- Single PlatformIO project simplifies build
- Conditional compilation based on board environment
- Easy to maintain consistency

**Alternative considered:**
- Separate projects: Risk of protocol drift, harder to keep in sync

### Decision 9: Error Handling Strategy
**Choice:** 
- Serial logging for debugging
- LED error patterns (rapid blink = communication error)
- Automatic retry for critical messages
- Watchdog timer for crash recovery

**Rationale:**
- Serial available during development
- Visual feedback for field troubleshooting
- Graceful degradation vs hard failures
- Watchdog prevents permanent hang states

### Decision 10: Battery Power Management
**Choice:** LiPo batteries for buzzer nodes, light sleep mode between events
**Rationale:**
- 1100mAh 3.7V LiPo provides ~4 hours of operation per buzzer
- ESP32 light sleep (5-10mA) vs active (80-240mA) extends battery life significantly
- Wake on button press using GPIO interrupt
- LED off during sleep saves additional power
- Main controller USB powered (always on, no sleep needed)

**Power consumption breakdown (per buzzer node):**
- Active mode (LED on, waiting): ~100mA → 11 hours theoretical
- Light sleep mode: ~8mA → 137 hours theoretical
- Mixed usage (90% sleep, 10% active): ~18mA → ~61 hours theoretical
- Realistic quiz usage (active during rounds): ~4-6 hours

**Implementation:**
```cpp
// Enter light sleep when idle
esp_sleep_enable_gpio_wakeup();
gpio_wakeup_enable(BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
esp_light_sleep_start();
```

**Alternative considered:**
- No sleep mode: Simpler but only ~11 hours battery life
- Deep sleep: Loses ESP-NOW connection, too slow to wake (~1s)

### Decision 11: MAC Address Management
**Choice:** Custom MAC addresses set via esp_wifi_set_mac()
**Rationale:**
- ESP32 supports setting custom MAC addresses via ESP-IDF API
- Set predictable MACs on buzzer nodes: `{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01}` through `0x04`
- Main controller can hardcode these known addresses
- No need to read and manually configure MAC addresses
- Simplifies deployment: flash same main controller firmware to any board

**Implementation:**
```cpp
// Buzzer node initialization
uint8_t customMAC[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, NODE_ID};
esp_wifi_set_mac(WIFI_IF_STA, customMAC);
```

**Alternative considered:**
- Use factory MACs and hardcode after reading: More manual work, error-prone
- Auto-discovery protocol: Adds complexity, not needed for fixed 4-node setup

## Risks / Trade-offs

### Risk 1: ESP-NOW Message Loss
**Impact:** Missed button press or LED command
**Mitigation:**
- Implement acknowledgment messages for button presses
- Retry button press transmission up to 3 times with 10ms interval
- Main controller sends periodic LED state refresh (every 5 seconds)
- LED commands don't need ACK (visual state is self-correcting)

### Risk 2: Simultaneous Button Presses
**Impact:** Ambiguous first-press determination
**Mitigation:**
- Main controller uses receive timestamp (micros()) as tiebreaker
- Add sequence number to messages
- Accept that <1ms simultaneous presses may have arbitrary winner (acceptable for quiz games)

### Risk 3: Main Controller Failure
**Impact:** Entire system non-functional
**Mitigation:**
- Watchdog timer auto-resets controller on crash
- Buzzer nodes show error pattern if no communication for >5 seconds
- Manual reset button on main controller
- Keep main controller firmware simple to reduce crash risk

### Risk 4: Power Supply Noise
**Impact:** False button triggers or ESP-NOW interference
**Mitigation:**
- Hardware: Add decoupling capacitors (100nF + 10uF) per board
- Software: Button debouncing filters electrical noise
- ESP-NOW uses 2.4GHz with automatic channel selection (less interference than WiFi)

### Risk 5: Serial Communication Issues
**Impact:** PC misses messages or receives corrupted data
**Mitigation:**
- Use higher baud rate (115200) for reliability
- Add newline terminators for message framing
- Queue messages if multiple occur rapidly (max 10)
- PC software implements timeout and retry logic
- Test with target quiz software

### Risk 6: Memory Constraints
**Impact:** ESP32 runs out of RAM (320KB limit)
**Mitigation:**
- Profile memory usage during development
- Keep message buffers small (max 10 queued messages)
- Use static allocation (no dynamic memory)
- Monitor stack usage with Serial logging

### Risk 7: Battery Life
**Impact:** Buzzer nodes run out of power during quiz event
**Mitigation:**
- Implement light sleep mode to extend battery life to 4+ hours
- Test actual battery consumption under realistic usage
- Visual low battery warning (LED flicker pattern)
- Design for easy battery replacement (accessible connector)
- Document expected battery life in user guide
- Consider voltage monitoring for low battery detection (optional v1)

## Migration Plan
N/A - This is the initial implementation, no migration needed.

**Deployment Steps:**
1. Flash buzzer node firmware to 4 boards with NODE_ID build flags (1-4)
   - `pio run -e buzzer_node_1 -t upload` (sets NODE_ID=1, custom MAC :01)
   - `pio run -e buzzer_node_2 -t upload` (sets NODE_ID=2, custom MAC :02)
   - `pio run -e buzzer_node_3 -t upload` (sets NODE_ID=3, custom MAC :03)
   - `pio run -e buzzer_node_4 -t upload` (sets NODE_ID=4, custom MAC :04)
2. Label each buzzer board physically ("Buzzer 1" through "Buzzer 4")
3. Flash main controller firmware to 1 board, label as "MAIN"
4. Power up all boards and verify serial output shows custom MAC addresses
5. Test communication by pressing each buzzer
6. Connect main board to PC via USB and verify serial messages
7. Test full workflow with quiz software

**Rollback:**
- Keep original simple firmware available
- Each board can be reflashed individually via USB
- No persistent state, rollback is just reflashing firmware

## Open Questions

### Resolved

1. **Speaker placement:** ✅ DECIDED
   - Each buzzer node will get a speaker (4 speakers total)
   - Main controller will NOT have a speaker
   - Feature will be implemented in a future proposal (out of scope for initial implementation)
   - Design will leave GPIO pins available for future speaker integration

2. **MAC address configuration:** ✅ DECIDED
   - Use hardcoded MAC addresses in main controller firmware
   - Investigate ESP32 capability to set custom MAC addresses on buzzer nodes
   - If custom MAC setting is possible: configure each buzzer with predictable MAC (e.g., AA:BB:CC:DD:EE:01 through :04)
   - If not possible: Read actual MAC addresses from buzzer nodes via serial output and hardcode in main controller
   - No auto-discovery for initial implementation (can add later if needed)

3. **Power supply:** ✅ DECIDED
   - Main controller: USB powered (5V from PC)
   - Buzzer nodes: Battery powered (1100mAh 4.1Wh 3.7V LiPo per node)
   - Expected battery life: ~4 hours continuous operation
   - Power optimization: Use ESP32 light sleep between button presses
   - Low battery detection: Monitor voltage and indicate via LED pattern (optional v1 feature)

4. **Buzzer node ID configuration:** ✅ DECIDED
   - Hardcoded in firmware during compilation
   - Use PlatformIO build flags to set node ID (e.g., `-DNODE_ID=1`)
   - Physical labeling on each board (stickers: "Buzzer 1" through "Buzzer 4")
   - Can add DIP switch support in future if needed

5. **LED blink rate:** ✅ DECIDED
   - Start with 2Hz (500ms on, 500ms off) for selected buzzer
   - Use #define constant in config.h for easy tuning
   - Can be adjusted based on user testing feedback
