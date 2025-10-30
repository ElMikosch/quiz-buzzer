## MODIFIED Requirements

### Requirement: State Persistence
The game state SHALL be maintained in volatile memory and reset on power cycle, with automatic recovery for individual buzzer reconnections.

#### Scenario: Power cycle resets state
- **WHEN** the main controller loses power
- **AND** power is restored
- **THEN** the game SHALL initialize to ready state
- **AND** all previous state SHALL be cleared

#### Scenario: Buzzer reconnection preserves state
- **WHEN** a single buzzer node loses power or connection
- **AND** the main controller remains powered
- **THEN** the game state SHALL be preserved
- **AND** the reconnecting buzzer SHALL sync to current state
- **AND** other buzzers SHALL maintain their state
- **AND** gameplay SHALL continue normally

#### Scenario: Multiple buzzers reconnect
- **WHEN** multiple buzzer nodes reconnect simultaneously
- **THEN** each SHALL receive state sync independently
- **AND** the game state SHALL remain consistent
- **AND** all reconnected buzzers SHALL match current state
