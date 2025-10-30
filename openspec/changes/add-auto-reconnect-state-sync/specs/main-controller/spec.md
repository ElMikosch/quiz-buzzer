## ADDED Requirements

### Requirement: Heartbeat Broadcasting
The main controller SHALL broadcast periodic heartbeat messages to all buzzer nodes.

#### Scenario: Periodic heartbeat transmission
- **WHEN** the controller is operating
- **THEN** it SHALL broadcast MSG_HEARTBEAT every 2000ms
- **AND** use ESP-NOW broadcast address (FF:FF:FF:FF:FF:FF)
- **AND** include timestamp in heartbeat message

#### Scenario: Heartbeat during all game states
- **WHEN** heartbeat interval expires
- **AND** game is in any state (ready/locked/partial lockout)
- **THEN** the heartbeat SHALL be sent
- **AND** NOT interfere with other message processing

### Requirement: Connection Status Tracking
The main controller SHALL track connection status of each buzzer node.

#### Scenario: Track last-seen timestamp
- **WHEN** receiving any message from a buzzer node
- **THEN** the controller SHALL update that node's last-seen timestamp
- **AND** mark the node as connected if previously disconnected

#### Scenario: Detect node disconnection
- **WHEN** no message received from a node for 5000ms
- **THEN** the controller SHALL mark the node as disconnected
- **AND** log disconnection event to Serial with node ID
- **AND** continue normal operation for other nodes

#### Scenario: Detect node reconnection
- **WHEN** receiving message from previously disconnected node
- **THEN** the controller SHALL mark node as connected
- **AND** log reconnection event to Serial with node ID
- **AND** wait for state sync request

### Requirement: State Synchronization Protocol
The main controller SHALL respond to state sync requests with current game state.

#### Scenario: Receive state sync request
- **WHEN** receiving MSG_STATE_REQUEST from a buzzer node
- **THEN** the controller SHALL determine correct LED state for that node
- **AND** send MSG_STATE_SYNC with current game state
- **AND** include locked buzzer bitmask and selected buzzer ID
- **AND** log state sync event to Serial

#### Scenario: State sync in ready state
- **WHEN** node requests state sync
- **AND** game is in ready state
- **THEN** the controller SHALL send LED_ON for all nodes
- **AND** indicate no buzzers are locked or selected

#### Scenario: State sync in locked state
- **WHEN** node requests state sync
- **AND** game is in locked state
- **THEN** the controller SHALL send LED_BLINK for selected node
- **AND** send LED_OFF for all other nodes
- **AND** include selected buzzer ID in state message

#### Scenario: State sync in partial lockout
- **WHEN** node requests state sync
- **AND** game is in partial lockout state
- **THEN** the controller SHALL send LED_BLINK for selected node
- **AND** send LED_OFF for locked-out nodes
- **AND** send LED_ON for remaining active nodes
- **AND** include locked buzzer bitmask in state message

### Requirement: Reconnection Event Logging
The main controller SHALL log connection status changes via USB Serial.

#### Scenario: Log disconnection event
- **WHEN** a buzzer node disconnects (timeout)
- **THEN** the controller SHALL send "DISCONNECT:<node_id>\n" to serial
- **AND** include timestamp

#### Scenario: Log reconnection event
- **WHEN** a buzzer node reconnects
- **THEN** the controller SHALL send "RECONNECT:<node_id>\n" to serial
- **AND** include timestamp

#### Scenario: Log state sync event
- **WHEN** sending state sync to a reconnected node
- **THEN** the controller SHALL send "STATE_SYNC:<node_id>\n" to serial
- **AND** include current game state in log
