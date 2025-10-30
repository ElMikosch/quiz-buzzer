# Game State Machine

## States

The main controller manages the following game states:

### STATE_READY
**Description**: System is ready for button presses from any buzzer.

**Behavior**:
- All active buzzers have their LEDs **ON** (solid)
- Locked-out buzzers have their LEDs **OFF**
- First button press transitions to STATE_LOCKED
- No buzzer is currently selected

**LED Pattern**: All active = solid ON, locked = OFF

**Transitions**:
- Button press → STATE_LOCKED (set selectedBuzzer)
- Already in ready → stay in STATE_READY

---

### STATE_LOCKED
**Description**: One buzzer has been pressed, all others are locked out.

**Behavior**:
- Selected buzzer LED is **BLINKING** at 2Hz (500ms on/off)
- All other buzzer LEDs are **OFF**
- No additional button presses are accepted
- Game host must press CORRECT or WRONG button

**LED Pattern**: Selected = blink, others = OFF

**Transitions**:
- CORRECT button → STATE_READY (clear selection and lockouts)
- WRONG button → STATE_PARTIAL_LOCKOUT (lock current buzzer)

---

### STATE_PARTIAL_LOCKOUT
**Description**: One or more buzzers gave wrong answers and are locked out. Remaining buzzers can still try.

**Behavior**:
- Previously selected buzzer is added to locked-out list
- LEDs update: locked buzzers **OFF**, active buzzers **ON**
- Next button press from non-locked buzzer → STATE_LOCKED
- If all buzzers become locked out → auto-reset to STATE_READY

**LED Pattern**: Locked = OFF, active = solid ON

**Transitions**:
- Button press from active buzzer → STATE_LOCKED
- All buzzers locked → STATE_READY (auto-reset)
- CORRECT button → STATE_READY (clear all)
- RESET button → STATE_READY (clear all)

---

## State Transitions

```
         ┌──────────────┐
    ┌───►│ STATE_READY  │◄────┐
    │    │ (All LEDs ON)│     │
    │    └──────┬───────┘     │
    │           │              │
    │      Button Press        │
    │           │              │
    │           ▼              │
    │    ┌──────────────┐     │
    │    │STATE_LOCKED  │     │
    │    │(Selected     │     │
    │    │ Blinks)      │     │
    │    └──────┬───────┘     │
    │           │              │
    │    ┌──────┴────────┐    │
    │    │               │    │
    │  WRONG          CORRECT │
    │    │               │    │
    │    ▼               └────┘
    │ ┌──────────────────┐
    │ │STATE_PARTIAL_    │
    │ │  LOCKOUT         │
    │ │(Wrong buzzer OFF,│
    │ │ others ON)       │
    │ └──────┬───────────┘
    │        │
    │   Button Press
    │   (active buzzer)
    │        │
    └────────┘
```

## State Variables

| Variable | Type | Description |
|----------|------|-------------|
| `currentState` | `GameState` | Current state (READY/LOCKED/PARTIAL_LOCKOUT) |
| `selectedBuzzer` | `uint8_t` | Currently selected buzzer (1-4), or 0 if none |
| `lockedBuzzers` | `uint8_t` | Bitmask of locked buzzers (bit 0-3 for buzzers 1-4) |
| `lastPressTime` | `uint32_t` | Timestamp of last press (for tie-breaking) |
| `nodeLastSeen[]` | `unsigned long[4]` | Timestamp of last message from each node |
| `nodeConnected[]` | `bool[4]` | Connection status for each node |

## Connection Monitoring & State Recovery

The system includes automatic connection monitoring to handle buzzer node disconnections (power cycles, network issues, etc.):

### Heartbeat System
- Main controller broadcasts heartbeat every **2 seconds**
- Buzzer nodes track time since last heartbeat
- Timeout after **5 seconds** without heartbeat
- Disconnected buzzers show **rapid LED blink** (10Hz / 100ms interval)

### Reconnection Flow
When a buzzer node reconnects (after timeout or power cycle):

1. **Detection**: Node receives heartbeat after being disconnected
2. **State Request**: Node sends `MSG_STATE_REQUEST` to main controller
3. **State Sync**: Main controller sends `MSG_STATE_SYNC` with packed game state:
   - Bits 0-3: `lockedBuzzers` bitmask
   - Bits 4-6: `selectedBuzzer` (0 = none, 1-4 = buzzer ID)
4. **LED Restoration**: Node unpacks state and restores correct LED behavior:
   - If node is selected → LED_BLINK (2Hz)
   - If node is locked → LED_OFF
   - Otherwise → LED_ON (ready)

### State Preservation During Disconnection
- Main controller maintains game state regardless of node connectivity
- Locked buzzers remain locked even if they disconnect/reconnect
- Selected buzzer status is preserved
- When reconnecting, nodes synchronize to current game state

### Connection Status Tracking
- Main controller tracks last-seen timestamp for each node
- `DISCONNECT:<id>` logged when 5s timeout detected
- `RECONNECT:<id>` logged when node sends any message after timeout
- Connection status does not affect game state logic

## Bitmask Operations

The `lockedBuzzers` variable uses a bitmask to track which buzzers are locked:

```cpp
// Check if buzzer 2 is locked
if (lockedBuzzers & (1 << (2 - 1))) {
  // Buzzer 2 is locked
}

// Lock buzzer 3
lockedBuzzers |= (1 << (3 - 1));

// Clear all locks
lockedBuzzers = 0;

// Check if all buzzers locked (bits 0-3 all set)
if (lockedBuzzers == 0x0F) {
  // All 4 buzzers locked
}
```

## Control Actions

| Action | Button | Behavior |
|--------|--------|----------|
| **Correct Answer** | CTRL_BUTTON_CORRECT (GPIO 25) | Reset to STATE_READY, clear all locks and selection |
| **Wrong Answer** | CTRL_BUTTON_WRONG (GPIO 26) | Lock out selected buzzer, move to PARTIAL_LOCKOUT |
| **Full Reset** | CTRL_BUTTON_RESET (GPIO 27) | Force reset to STATE_READY from any state |

## Example Scenarios

### Scenario 1: Simple Correct Answer
1. Start in STATE_READY (all LEDs on)
2. Buzzer 2 presses → STATE_LOCKED (buzzer 2 blinks, others off)
3. Host presses CORRECT → STATE_READY (all LEDs on)

### Scenario 2: Wrong Answer, Then Correct
1. Start in STATE_READY
2. Buzzer 1 presses → STATE_LOCKED (buzzer 1 blinks)
3. Host presses WRONG → STATE_PARTIAL_LOCKOUT (buzzer 1 off, 2/3/4 on)
4. Buzzer 3 presses → STATE_LOCKED (buzzer 3 blinks, 1 stays off, 2/4 off)
5. Host presses CORRECT → STATE_READY (all LEDs on, locks cleared)

### Scenario 3: Multiple Wrong Answers
1. Start in STATE_READY
2. Buzzer 1 presses → STATE_LOCKED
3. WRONG → STATE_PARTIAL_LOCKOUT (1 locked)
4. Buzzer 2 presses → STATE_LOCKED
5. WRONG → STATE_PARTIAL_LOCKOUT (1,2 locked)
6. Buzzer 3 presses → STATE_LOCKED
7. WRONG → STATE_PARTIAL_LOCKOUT (1,2,3 locked)
8. Buzzer 4 presses → STATE_LOCKED (last buzzer)
9. WRONG → Auto-reset to STATE_READY (all buzzers locked, clear everything)

### Scenario 4: Emergency Reset
1. Any state
2. Host presses RESET → STATE_READY (force clear all state)

### Scenario 5: Buzzer Reconnection During Game
1. Start in STATE_READY (all LEDs on)
2. Buzzer 1 presses → STATE_LOCKED (buzzer 1 blinks)
3. Host presses WRONG → STATE_PARTIAL_LOCKOUT (buzzer 1 locked/off, 2/3/4 on)
4. Buzzer 2 power cycled (unplugged)
   - Main controller detects timeout after 5s → logs `DISCONNECT:2`
   - Buzzer 2 LED enters rapid blink (10Hz) during disconnection
   - Game state preserved: buzzer 1 still locked
5. Buzzer 2 powered back on
   - Receives heartbeat from controller
   - Sends `MSG_STATE_REQUEST`
   - Controller logs `RECONNECT:2`
   - Receives `MSG_STATE_SYNC` with: `lockedBuzzers=0x01` (bit 0 set), `selectedBuzzer=0`
   - LED restores to solid ON (ready state, since not locked or selected)
6. Buzzer 2 can now press button normally
7. Game continues as before reconnection
