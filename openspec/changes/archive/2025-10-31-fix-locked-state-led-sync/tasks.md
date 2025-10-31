# Implementation Tasks

## Status: ✅ IMPLEMENTED - Awaiting Hardware Testing

## Completed Tasks

### 1. ✅ Update protocol.h documentation
- **File**: `src/protocol.h:29-31`
- **Change**: Added documentation for bit 7 usage in MSG_STATE_SYNC value field
- **Details**: Documents bit layout for state sync messages

### 2. ✅ Modify sendStateSync() in controller.cpp
- **File**: `src/controller.cpp:177-179`
- **Change**: Set bit 7 when currentState == STATE_PARTIAL_LOCKOUT
- **Implementation**: `if (currentState == STATE_PARTIAL_LOCKOUT) { msg.value |= 0x80; }`

### 3. ✅ Update state sync handler in buzzer_node.cpp
- **File**: `src/buzzer_node.cpp:93,118-131`
- **Changes**:
  - Extract bit 7 to `isPartialLockout` boolean
  - Replace simple `isLocked` check with conditional logic:
    - If `isPartialLockout` is true: only turn OFF if explicitly in locked bitmask, otherwise ON
    - If `isPartialLockout` is false: all non-selected buzzers turn OFF (STATE_LOCKED behavior)

### 4. ✅ Build verification
- **Status**: Build successful
- **Details**: All environments compile without errors

## Pending Tasks

### 5. Hardware Testing
- **Status**: AWAITING USER TESTING
- **Test Scenarios**:
  1. Buzzer 1 presses → disconnect Buzzer 2 → reconnect Buzzer 2
     - Expected: Buzzer 2 LED stays OFF (STATE_LOCKED)
  2. Buzzer 1 presses → WRONG → disconnect Buzzer 1 → reconnect Buzzer 1
     - Expected: Buzzer 1 LED turns OFF (locked in PARTIAL_LOCKOUT)
  3. Buzzer 1 presses → WRONG → disconnect Buzzer 2/3/4 → reconnect them
     - Expected: Buzzer 2/3/4 LEDs turn ON (not locked in PARTIAL_LOCKOUT)

## Change Summary

**Problem**: Non-selected buzzers incorrectly turned ON when reconnecting during STATE_LOCKED.

**Solution**: Use bit 7 of state sync value field to distinguish between STATE_LOCKED (all non-selected OFF) and STATE_PARTIAL_LOCKOUT (only locked bitmask OFF).

**Files Modified**:
- `src/protocol.h` - Documentation update
- `src/controller.cpp` - Set bit 7 for PARTIAL_LOCKOUT
- `src/buzzer_node.cpp` - Check bit 7 and apply correct LED logic
