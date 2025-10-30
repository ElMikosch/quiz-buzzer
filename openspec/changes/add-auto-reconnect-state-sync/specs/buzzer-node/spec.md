## ADDED Requirements

### Requirement: Connection Monitoring
Each buzzer node SHALL monitor its connection to the main controller and detect disconnections.

#### Scenario: Normal heartbeat reception
- **WHEN** the node receives a heartbeat message from the main controller
- **THEN** it SHALL update its last-heartbeat timestamp
- **AND** maintain normal operation with current LED state

#### Scenario: Connection timeout detected
- **WHEN** no heartbeat received for CONNECTION_TIMEOUT_MS (5000ms)
- **THEN** the node SHALL enter disconnected state
- **AND** set LED to rapid blink (5Hz) to indicate error
- **AND** disable button presses
- **AND** log disconnection event to Serial

#### Scenario: Heartbeat received after timeout
- **WHEN** in disconnected state
- **AND** a heartbeat message is received
- **THEN** the node SHALL detect reconnection
- **AND** send MSG_STATE_REQUEST to main controller
- **AND** wait for state sync before updating LED
- **AND** log reconnection event to Serial

### Requirement: State Synchronization
Each buzzer node SHALL request and receive current game state when reconnecting.

#### Scenario: Request state on reconnection
- **WHEN** the node reconnects after timeout
- **THEN** it SHALL send MSG_STATE_REQUEST with its node ID
- **AND** wait up to 1000ms for MSG_STATE_SYNC response
- **AND** retry up to 3 times if no response

#### Scenario: Receive state sync message
- **WHEN** receiving MSG_STATE_SYNC from main controller
- **THEN** the node SHALL extract the LED state for its node ID
- **AND** immediately apply the correct LED state (on/off/blink)
- **AND** restore normal operation
- **AND** log state sync completion to Serial

#### Scenario: State sync timeout
- **WHEN** no MSG_STATE_SYNC received within 1000ms after request
- **THEN** the node SHALL continue rapid LED blink
- **AND** retry MSG_STATE_REQUEST
- **AND** log timeout to Serial

### Requirement: Power Cycle Recovery
Each buzzer node SHALL automatically recover correct state after power cycle.

#### Scenario: Power cycle during ready state
- **WHEN** node powers on
- **AND** main controller is in ready state
- **THEN** the node SHALL connect to main controller
- **AND** receive heartbeat within 5 seconds
- **AND** request state sync
- **AND** restore LED to on (ready state)

#### Scenario: Power cycle during locked state
- **WHEN** node powers on
- **AND** game is in locked state
- **THEN** the node SHALL request and receive current state
- **AND** restore correct LED (off if not selected, blink if selected)
- **AND** maintain proper button disable state

#### Scenario: Power cycle during partial lockout
- **WHEN** node powers on
- **AND** game is in partial lockout state
- **AND** this node is locked out
- **THEN** the node SHALL restore LED to off
- **AND** disable button presses
