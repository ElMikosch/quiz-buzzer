# Technical Design: Battery Usage Optimization

## Context

The quiz buzzer system runs on battery-powered ESP32 nodes that must operate throughout quiz sessions (30-60 minutes typical). Current implementation consumes excessive power due to:
- Full-speed CPU operation (240MHz)
- Always-active WiFi without modem sleep
- Continuous polling loop with minimal delay (1ms)
- No sleep modes between operations

Human reaction time (150-300ms average, 100ms for fast reflexes) provides headroom for modest latency additions while maintaining quiz fairness. Commercial quiz systems typically have 20-50ms latency; we target <50ms to stay competitive while optimizing battery life.

## Goals / Non-Goals

**Goals:**
- Reduce power consumption by 50-70% during active quiz sessions
- Maintain button press response time <50ms for fairness
- Ensure equal latency across all nodes (±5ms variance)
- Extend battery life from hours to full-day operation
- Preserve all existing functionality and LED behaviors

**Non-Goals:**
- Deep sleep modes (incompatible with sub-second response requirements)
- Battery monitoring/fuel gauge (future enhancement)
- Dynamic heartbeat adjustment based on activity
- Power optimization for main controller (remains USB-powered)
- Adaptive power modes based on battery level

## Decisions

### Decision 1: Light Sleep Instead of Deep Sleep
**Choice:** Use ESP32 light sleep mode with GPIO wake on button press.

**Rationale:**
- Light sleep wakes in ~3-5ms vs deep sleep's 100-300ms
- GPIO interrupt wake provides immediate response to button press
- WiFi and ESP-NOW connection state preserved (no reconnection delay)
- Timer wake allows periodic processing (LED updates, heartbeat checks)

**Alternatives considered:**
- Deep sleep: Rejected due to 100-300ms wake time violating <50ms requirement
- No sleep: Current approach, unacceptable battery consumption
- Active waiting with yield(): Minimal power savings compared to light sleep

### Decision 2: CPU Frequency Reduction to 80MHz
**Choice:** Reduce CPU from default 240MHz to 80MHz.

**Rationale:**
- ESP32 power consumption scales roughly linearly with frequency
- 80MHz provides 66% reduction from 240MHz (saves ~40% CPU power)
- More than adequate for simple embedded logic (GPIO, ESP-NOW, PWM)
- Arduino timing functions (millis/micros) remain accurate
- ESP-NOW and WiFi drivers work correctly at 80MHz

**Alternatives considered:**
- 160MHz: Less power savings, no performance benefit needed
- 40MHz: Potential WiFi/ESP-NOW reliability issues
- Dynamic scaling: Added complexity without clear benefit for this use case

### Decision 3: WiFi Modem Sleep Mode
**Choice:** Enable `WIFI_PS_MIN_MODEM` power save mode.

**Rationale:**
- WiFi radio is largest power consumer on ESP32
- Modem sleep shuts down RF between packets while maintaining connection
- Automatic wake on ESP-NOW TX/RX (transparent to application)
- Adds 5-10ms latency (acceptable within 50ms budget)
- No code changes needed beyond configuration call

**Alternatives considered:**
- `WIFI_PS_MAX_MODEM`: More aggressive sleep, but can cause packet loss
- `WIFI_PS_NONE`: Current default, no power savings
- Disable WiFi between heartbeats: Complex state management, connection reliability issues

### Decision 4: Dynamic Power Modes Based on Game State
**Choice:** Implement three power modes with different sleep durations:
- **ACTIVE** (ready state): 50ms sleep, GPIO wake enabled
- **LOW_POWER** (locked out): 100ms sleep, GPIO wake disabled
- **MINIMAL** (disconnected): 200ms sleep, GPIO wake disabled

**Rationale:**
- Ready state needs fast response, justify shorter sleep (50ms)
- Locked/disconnected states can sleep longer (button inactive)
- Breathing LED fade (20ms updates) works fine with 200ms wake interval
- State transitions occur naturally with LED commands
- Significant power savings during non-active periods

**Alternatives considered:**
- Single sleep duration: Misses optimization opportunity during locked/disconnected
- Five+ power modes: Complexity without proportional benefit
- Activity-based adaptation: Over-engineered for predictable quiz usage patterns

### Decision 5: Timer Wake as Fallback to GPIO Wake
**Choice:** Always configure both GPIO wake (button) and timer wake (backup) for light sleep.

**Rationale:**
- GPIO wake handles button presses immediately (<5ms)
- Timer wake ensures periodic processing even if no button press
- Required for LED updates (blink/fade), heartbeat monitoring, ESP-NOW RX
- 50ms timer wake in ready state ensures <50ms worst-case button latency
- Longer timers (100-200ms) acceptable when button inactive

**Alternatives considered:**
- GPIO wake only: Would miss heartbeats and LED updates
- Timer wake only: Higher power consumption, slower button response
- Event-driven ESP-NOW wake: Not reliably supported in light sleep

## Architecture

### Power State Machine

```
           [Startup]
               |
               v
        [MINIMAL MODE]
        LED: breathing
        Sleep: 200ms
        GPIO wake: disabled
               |
               | (receive heartbeat + state sync → ready)
               v
        [ACTIVE MODE] <----------+
        LED: solid on            |
        Sleep: 50ms              |
        GPIO wake: enabled       | (reset/correct button)
               |                 |
               | (wrong answer / locked out)
               v                 |
        [LOW_POWER MODE]         |
        LED: off                 |
        Sleep: 100ms             |
        GPIO wake: disabled -----+
```

### Loop Execution Flow

```
loop() iteration:
1. Check connection (heartbeat timeout)
2. Handle button press (if GPIO wake triggered)
3. Update LED (blink/fade state machine)
4. Determine current power mode from LED state
5. Configure sleep (GPIO + timer wake sources)
6. Enter light sleep
7. Wake on button press OR timer expiry
8. Repeat
```

### Timing Budget Analysis

| Operation | Time (ms) | Notes |
|-----------|-----------|-------|
| Light sleep wake | 3-5 | GPIO interrupt wake |
| Button debounce | 0-50 | Max debounce delay |
| ESP-NOW transmission | 5-10 | With modem sleep wake |
| Loop processing overhead | 1-2 | GPIO read, LED update |
| **Total worst case** | **59-67** | **Exceeds 50ms target** |

**Mitigation:** Reduce debounce delay from 50ms to 30ms in active mode, bringing total to 39-47ms.

## Risks / Trade-offs

### Risk 1: Light Sleep Compatibility with ESP-NOW Reception
**Risk:** ESP-NOW messages might not reliably wake ESP32 from light sleep.

**Mitigation:** 
- Timer wake every 50ms ensures messages processed within reasonable time
- Heartbeat interval (2000ms) >> sleep duration (50ms), plenty of margin
- Test message reception reliability extensively during implementation

### Risk 2: Timing Variance Across Nodes
**Risk:** Power mode transitions could cause temporary timing differences between nodes.

**Mitigation:**
- All nodes transition together (synchronized by main controller LED commands)
- Test simultaneous button presses during state transitions
- Log timing measurements to verify fairness (±5ms target)

### Risk 3: PWM/LED Behavior at Reduced CPU Frequency
**Risk:** LED PWM might flicker or lose precision at 80MHz.

**Mitigation:**
- LEDC peripheral uses independent clock, not affected by CPU frequency
- Test all LED patterns (solid, blink, fade) after frequency change
- Measure PWM frequency/duty cycle with oscilloscope if issues arise

### Risk 4: Reduced Button Debounce Time
**Risk:** 30ms debounce might allow switch bounce through, causing double presses.

**Mitigation:**
- Test with all 4 physical buzzer buttons extensively
- Monitor serial logs for unexpected multiple presses within short time
- Can increase debounce if bounce detected (trade latency vs reliability)
- Add "burst suppression" (ignore presses within 100ms of previous press)

## Migration Plan

**Phase 1: Non-Breaking Power Optimizations**
1. Add CPU frequency scaling (always safe, test timing)
2. Enable WiFi modem sleep (transparent to application)
3. Add telemetry: log current measurements, button latency

**Phase 2: Light Sleep Implementation**
1. Add light sleep with conservative 50ms duration in all modes
2. Test button responsiveness and message reception
3. Monitor for any missed heartbeats or state sync issues

**Phase 3: Dynamic Power Modes**
1. Implement power mode state machine
2. Add longer sleep durations for locked/disconnected states
3. Test state transitions and timing fairness

**Rollback:** Each phase can be individually disabled via config constants if issues arise.

## Open Questions

1. **Optimal debounce time:** Is 30ms sufficient, or do we need 40-50ms? Requires hardware testing.
2. **Battery capacity selection:** What mAh capacity should be recommended for target full-day operation? Depends on measured power consumption.
3. **Sleep duration tuning:** Are 50/100/200ms optimal, or should we adjust based on measured latency? Will iterate during testing.
4. **Main controller optimization:** Should main controller also reduce CPU frequency (doesn't affect battery life but reduces heat)? Could add in future.
