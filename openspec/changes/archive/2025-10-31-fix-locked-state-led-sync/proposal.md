# Change Proposal: fix-locked-state-led-sync

## Why

When a buzzer node reconnects during STATE_LOCKED (one buzzer pressed, all others locked out), non-selected buzzers incorrectly turn their LED ON. This happens because:

1. In STATE_LOCKED, the main controller doesn't populate the `lockedBuzzers` bitmask (it remains 0)
2. The state sync message only sends `lockedBuzzers` and `selectedBuzzer`
3. The buzzer node logic assumes: if not locked and not selected → LED ON (ready state)
4. But in STATE_LOCKED, ALL non-selected buzzers should be OFF

This works correctly for PARTIAL_LOCKOUT because wrong-answer buzzers ARE added to the `lockedBuzzers` bitmask. The issue is that STATE_LOCKED and PARTIAL_LOCKOUT are semantically different but share the same state sync protocol, which doesn't distinguish between them.

## What Changes

- Use bit 7 of the state sync `value` field to encode game state (currently unused)
  - Bit 7 = 0: STATE_LOCKED or STATE_READY (all non-selected buzzers are locked out)
  - Bit 7 = 1: STATE_PARTIAL_LOCKOUT (only explicit `lockedBuzzers` are locked, others are active)
- Update main controller's `sendStateSync()` to set bit 7 when in PARTIAL_LOCKOUT
- Update buzzer node's state sync handler to check bit 7 and apply correct LED logic:
  - If bit 7 = 0 and selectedBuzzer != 0: Only selected buzzer can have LED on, all others OFF
  - If bit 7 = 1: Use existing logic (selected=BLINK, locked=OFF, others=ON)
  - If selectedBuzzer == 0: All LEDs ON (ready state)

## Impact

- **Affected specs**: main-controller, buzzer-node
- **Affected code**:
  - `src/controller.cpp` - Update `sendStateSync()` to set bit 7 based on `currentState`
  - `src/buzzer_node.cpp` - Update state sync handler to check bit 7 for game state
  - `src/protocol.h` - Update comments to document bit 7 usage
- **Benefits**:
  - Correct LED behavior when non-selected buzzer reconnects during STATE_LOCKED
  - Preserves correct behavior for PARTIAL_LOCKOUT reconnections
  - No breaking changes to existing protocol structure
  - Uses currently unused bit, no protocol version change needed
- **Risks**: 
  - Low - uses reserved bit as intended
  - Backwards compatibility: old buzzer node firmware would ignore bit 7 (existing bug persists)
  - Future firmware updates will fix the issue as nodes are updated
