## Why

When a buzzer node temporarily loses power or WiFi connection during gameplay, it currently does not recover automatically. When reconnected, the buzzer's LED state does not match the actual game state (e.g., it shows "ready" when the game is locked). This creates confusion and requires manual reset to restore proper operation.

## What Changes

- Add heartbeat mechanism between main controller and buzzer nodes
- Implement automatic state synchronization on reconnection
- Add connection status tracking on both controller and nodes
- Send full game state to reconnecting buzzers
- Handle edge cases: power cycle during lockout, missed state transitions

## Impact

- Affected specs: buzzer-node, main-controller, game-state
- Affected code:
  - `src/controller.cpp` - Add heartbeat broadcast, track node connectivity, state sync on reconnect
  - `src/buzzer_node.cpp` - Add heartbeat monitoring, request state on reconnect
  - `src/protocol.h` - Add new message types (MSG_HEARTBEAT, MSG_STATE_REQUEST, MSG_STATE_SYNC)
  - `src/config.h` - Add timing constants (heartbeat interval, timeout threshold)
