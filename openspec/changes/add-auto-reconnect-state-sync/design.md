## Context

The current system lacks connection monitoring between the main controller and buzzer nodes. When a buzzer node loses power or WiFi connectivity, it cannot detect the disconnection or automatically recover when reconnected. This requires manual intervention (reset button) to restore proper operation.

ESP-NOW provides no built-in connection tracking - it's a stateless protocol. We need application-level heartbeat and state sync mechanisms.

## Goals / Non-Goals

**Goals:**
- Automatic detection of disconnected buzzer nodes
- Seamless recovery when nodes reconnect without manual reset
- Preserve game state during individual node failures
- Clear visual feedback (LED) when node is disconnected
- Serial logging of connection events for debugging

**Non-Goals:**
- Handling main controller power cycle (game state still resets)
- Persistent storage of game state (remains volatile)
- Network mesh/multi-hop communication
- Automatic firmware updates on reconnect

## Decisions

### Decision: Heartbeat-Based Connection Monitoring

**What:** Main controller broadcasts heartbeat every 2 seconds. Nodes detect timeout after 5 seconds without heartbeat.

**Why:**
- Simple, proven pattern for stateless protocols
- Low overhead (1 small message every 2 seconds)
- Fast detection (5 second timeout is acceptable for quiz application)
- Broadcast reduces message count vs. unicast to each node

**Alternatives considered:**
- Bidirectional ping/pong: Higher message volume (4x), no benefit for our use case
- TCP-like connection tracking: Over-engineered, ESP-NOW is unreliable by design
- No heartbeat, detect on message failure: Cannot detect silent failures (power loss)

### Decision: Pull-Based State Sync on Reconnect

**What:** When node detects reconnection (heartbeat after timeout), it requests state from controller.

**Why:**
- Node knows when it needs state (reconnection event)
- Controller doesn't waste bandwidth pushing state unnecessarily
- Handles race conditions better (node may receive heartbeat before fully initialized)

**Alternatives considered:**
- Push on reconnect: Controller must detect reconnection, risk of missed sync if node not ready
- Periodic state broadcast: Wastes bandwidth, no benefit during stable operation

### Decision: Single State Sync Message with Full Game State

**What:** MSG_STATE_SYNC includes current game state (selected buzzer, locked bitmask, LED states for all nodes).

**Why:**
- Atomic update prevents inconsistent state
- Simple protocol (one message type)
- Handles all edge cases (lockout, partial lockout, ready)

**Alternatives considered:**
- Per-node LED commands: Multiple messages, race conditions possible
- Stateful protocol: Too complex for simple quiz buzzer

### Decision: Rapid LED Blink for Disconnected State

**What:** When node detects timeout, blink LED at 5Hz (100ms on/off).

**Why:**
- Clear visual distinction from other states (ready=solid, selected=2Hz, locked=off)
- Alerts players that node is malfunctioning
- Fast enough to be noticed immediately

**Alternatives considered:**
- LED off: Confusing, looks like lockout
- Same as ready (solid): No indication of problem

## Timing Analysis

### Message Flow Timeline

**Normal Operation (per 2s):**
```
T+0ms:    Controller broadcasts heartbeat
T+<10ms:  All nodes receive heartbeat, update timestamp
T+2000ms: Next heartbeat
```

**Reconnection Flow:**
```
T+0ms:    Node loses power/connection
T+5000ms: Node detects timeout (no heartbeat)
T+5001ms: Node sets LED to rapid blink, disables button
T+6000ms: Node power restored, boots up
T+6500ms: Node receives heartbeat
T+6501ms: Node sends MSG_STATE_REQUEST
T+6520ms: Controller receives request, sends MSG_STATE_SYNC
T+6540ms: Node receives sync, restores correct LED state
```

**Worst-case detection time:** 5 seconds (timeout threshold)  
**Worst-case recovery time:** ~500ms after reconnect (heartbeat interval + round-trip)

### Memory Impact

**Per-node state on controller:**
- last_seen: 4 bytes (uint32_t)
- connected: 1 byte (bool)
- Total: 5 bytes × 4 nodes = 20 bytes

**New message types:**
- MSG_HEARTBEAT: 12 bytes (BuzzerMessage struct)
- MSG_STATE_REQUEST: 12 bytes
- MSG_STATE_SYNC: 12 bytes (reuse existing value field for bitmask)

**Total overhead:** ~20 bytes RAM, negligible flash

## Risks / Trade-offs

### Risk: Heartbeat packet loss causes false disconnection

**Mitigation:** 5-second timeout allows for 2 missed heartbeats (2s interval). ESP-NOW is reliable within range.

**Fallback:** Manual reset button still available if false disconnection occurs.

### Risk: Multiple nodes reconnecting simultaneously

**Mitigation:** State sync is per-node, controller handles requests independently. No shared state locks needed.

**Testing:** Explicitly test in validation (task 5.5).

### Risk: Race condition - heartbeat arrives before node ready

**Mitigation:** Node always requests state on reconnection. If heartbeat arrives before node ready, node will timeout again and retry.

### Risk: State sync message lost

**Mitigation:** Node retries state request up to 3 times with 1s timeout. After 3 failures, continues rapid blink until manual reset.

## Migration Plan

### Implementation Order
1. Protocol updates (MSG types)
2. Configuration constants
3. Controller heartbeat broadcast (non-breaking, nodes ignore unknown messages)
4. Controller state tracking and sync handler
5. Node heartbeat monitoring and state request
6. Testing and validation

### Backward Compatibility
- Old nodes ignore new message types (no handler registered)
- New controller can coexist with old nodes (heartbeat is broadcast, ignored if not handled)
- **Recommendation:** Update all nodes simultaneously in deployment for consistent behavior

### Rollback
- If issues found, flash previous firmware
- No persistent state changes, immediate rollback possible

## Open Questions

None - design is straightforward application of proven patterns.
