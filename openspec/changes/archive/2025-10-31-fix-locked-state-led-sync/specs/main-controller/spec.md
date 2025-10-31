# main-controller Spec Delta

## MODIFIED Requirements

### Requirement: State Synchronization Protocol
The main controller SHALL respond to state sync requests with current game state, including game state mode to distinguish between full lockout and partial lockout states.

#### Scenario: State sync message format (MODIFIED)
- **WHEN** sending MSG_STATE_SYNC to a buzzer node
- **THEN** the controller SHALL pack game state into the value field as follows:
  - Bits 0-3: locked buzzers bitmask (bit 0 = buzzer 1, bit 1 = buzzer 2, etc.)
  - Bits 4-6: selected buzzer ID (0-4, where 0 = none selected)
  - Bit 7: game state mode (0 = LOCKED/READY, 1 = PARTIAL_LOCKOUT)
- **AND** bit 7 SHALL be set to 1 when in STATE_PARTIAL_LOCKOUT
- **AND** bit 7 SHALL be set to 0 when in STATE_LOCKED or STATE_READY

#### Scenario: State sync in locked state (MODIFIED)
- **WHEN** node requests state sync
- **AND** game is in locked state
- **THEN** the controller SHALL send state sync with bit 7 = 0 (locked mode)
- **AND** send selectedBuzzer ID in bits 4-6
- **AND** send lockedBuzzers bitmask in bits 0-3 (may be 0x00)
- **AND** the buzzer node will interpret: selected buzzer gets BLINK, all others get OFF

#### Scenario: State sync in partial lockout (MODIFIED)
- **WHEN** node requests state sync
- **AND** game is in partial lockout state
- **THEN** the controller SHALL send state sync with bit 7 = 1 (partial lockout mode)
- **AND** send selectedBuzzer ID in bits 4-6
- **AND** send lockedBuzzers bitmask in bits 0-3
- **AND** the buzzer node will interpret: selected gets BLINK, locked get OFF, others get ON

#### Scenario: State sync in ready state (ADDED)
- **WHEN** node requests state sync
- **AND** game is in ready state
- **THEN** the controller SHALL send state sync with bit 7 = 0
- **AND** send selectedBuzzer = 0 in bits 4-6
- **AND** send lockedBuzzers = 0x00 in bits 0-3
- **AND** the buzzer node will interpret: all buzzers get ON
