# buzzer-node Spec Delta

## MODIFIED Requirements

### Requirement: State Synchronization
Each buzzer node SHALL request and receive current game state when reconnecting, interpreting the game state mode to determine correct LED behavior.

#### Scenario: Receive state sync message (MODIFIED)
- **WHEN** receiving MSG_STATE_SYNC from main controller
- **THEN** the node SHALL extract game state from the value field:
  - Bits 0-3: locked buzzers bitmask
  - Bits 4-6: selected buzzer ID (0 = none)
  - Bit 7: game state mode (0 = LOCKED/READY, 1 = PARTIAL_LOCKOUT)
- **AND** determine if this node is locked: `isLocked = lockedBuzzers & (1 << (NODE_ID - 1))`
- **AND** determine if this node is selected: `isSelected = (selectedBuzzer == NODE_ID)`
- **AND** determine game state mode: `isPartialLockout = (value & 0x80) != 0`

#### Scenario: State sync LED logic for locked mode (ADDED)
- **WHEN** receiving state sync
- **AND** bit 7 = 0 (locked/ready mode)
- **AND** selectedBuzzer != 0 (locked state)
- **THEN** if this node is selected: set LED to BLINK (two-stage pattern)
- **AND** if this node is not selected: set LED to OFF
- **AND** restore normal operation

#### Scenario: State sync LED logic for partial lockout mode (ADDED)
- **WHEN** receiving state sync
- **AND** bit 7 = 1 (partial lockout mode)
- **THEN** if this node is selected: set LED to BLINK (two-stage pattern)
- **AND** if this node is in lockedBuzzers bitmask: set LED to OFF
- **AND** if this node is neither selected nor locked: set LED to ON
- **AND** restore normal operation

#### Scenario: State sync LED logic for ready state (ADDED)
- **WHEN** receiving state sync
- **AND** bit 7 = 0 (locked/ready mode)
- **AND** selectedBuzzer == 0 (ready state)
- **THEN** set LED to ON for all nodes
- **AND** restore normal operation
