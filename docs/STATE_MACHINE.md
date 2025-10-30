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
