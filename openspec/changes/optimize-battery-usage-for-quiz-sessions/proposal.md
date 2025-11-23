# Battery Usage Optimization for Quiz Sessions

## Why
The buzzer nodes run exclusively on battery power during quiz sessions, but currently consume excessive power due to always-active WiFi, full-speed CPU operation, and continuous polling loops. This results in short battery life and frequent recharging. However, human reaction times (150-300ms) and quiz system requirements allow for moderate latency (30-80ms) as long as the first button press is always detected correctly and fairly across all nodes.

## What Changes
- **Add light sleep mode** to buzzer nodes between loop iterations with GPIO interrupt wake on button press
- **Add CPU frequency scaling** from 240MHz to 80MHz during normal operation (40% less power)
- **Add WiFi modem sleep** to reduce power consumption between ESP-NOW transmissions
- **Add dynamic power modes** based on game state (ready vs locked/disconnected)
- **Maintain response time guarantee** of <50ms button press detection for quiz fairness
- **Add power state transitions** for entering/exiting low-power modes based on game events

## Impact
- **Affected specs**: buzzer-node, main-controller
- **Affected code**: 
  - `src/buzzer_node.cpp`: Add power management, sleep modes, interrupt handlers
  - `src/config.h`: Add power management constants and thresholds
  - `src/controller.cpp`: Optimize heartbeat timing for power efficiency
- **Expected outcomes**:
  - 50-70% reduction in power consumption during quiz sessions
  - Maintained <50ms button press response time (well within human reaction time variance)
  - Equal latency across all nodes preserving quiz fairness
  - Extended battery life from hours to full-day operation
- **Non-goals**:
  - Deep sleep modes (incompatible with sub-second wake requirements)
  - Battery monitoring/reporting (future enhancement)
  - Dynamic heartbeat adjustment (keep simple for v1)
