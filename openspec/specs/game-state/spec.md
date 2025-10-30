# game-state Specification

## Purpose
TBD - created by archiving change add-multi-board-buzzer-system. Update Purpose after archive.
## Requirements
### Requirement: Ready State
The game SHALL start in ready state where all buzzers can be pressed.

#### Scenario: System initialization to ready
- **WHEN** the system powers on or is reset
- **THEN** the game state SHALL be set to ready
- **AND** all 4 buzzers SHALL be active (can press)
- **AND** all 4 buzzer LEDs SHALL be lit
- **AND** no buzzer is selected

#### Scenario: Button press in ready state
- **WHEN** the game is in ready state
- **AND** a buzzer button is pressed
- **THEN** the game SHALL transition to locked state
- **AND** record the pressing buzzer as selected
- **AND** send the buzzer key (1-4) to PC
- **AND** make the selected buzzer LED blink
- **AND** turn off all other buzzer LEDs
- **AND** disable all buzzer buttons

### Requirement: Locked State
When a buzzer is selected, the game SHALL prevent further buzzer presses until an answer is judged.

#### Scenario: Correct answer from locked state
- **WHEN** in locked state with a selected buzzer
- **AND** the correct answer button is pressed
- **THEN** the game SHALL transition to ready state
- **AND** send 'C' key to PC
- **AND** turn on all buzzer LEDs
- **AND** enable all buzzer buttons
- **AND** clear the selected buzzer

#### Scenario: Wrong answer from locked state
- **WHEN** in locked state with a selected buzzer
- **AND** the wrong answer button is pressed
- **AND** at least 2 buzzers are still active
- **THEN** the game SHALL transition to partial lockout state
- **AND** send 'W' key to PC
- **AND** lock out the selected buzzer
- **AND** turn off the locked buzzer LED
- **AND** turn on remaining active buzzer LEDs
- **AND** enable remaining active buzzer buttons

#### Scenario: Wrong answer with only one buzzer remaining
- **WHEN** in locked state with a selected buzzer
- **AND** the wrong answer button is pressed
- **AND** only 1 buzzer is still active
- **THEN** the game SHALL lock out the last buzzer
- **AND** send 'W' key to PC
- **AND** remain in locked state (all buzzers locked)
- **AND** all LEDs SHALL be off

### Requirement: Partial Lockout State
After a wrong answer, the game SHALL allow remaining active buzzers to answer while keeping wrong buzzers locked out.

#### Scenario: Another buzzer press in partial lockout
- **WHEN** in partial lockout state
- **AND** an active buzzer button is pressed
- **THEN** the game SHALL transition to locked state
- **AND** record the new buzzer as selected
- **AND** send the buzzer key (1-4) to PC
- **AND** make the selected buzzer LED blink
- **AND** turn off active buzzer LEDs (keep locked LEDs off)

#### Scenario: Correct answer from partial lockout
- **WHEN** in partial lockout state with a selected buzzer
- **AND** the correct answer button is pressed
- **THEN** the game SHALL transition to ready state
- **AND** send 'C' key to PC
- **AND** unlock all buzzers
- **AND** turn on all buzzer LEDs
- **AND** enable all buzzer buttons

#### Scenario: Another wrong answer in partial lockout
- **WHEN** in partial lockout state with a selected buzzer
- **AND** the wrong answer button is pressed
- **THEN** the game SHALL lock out the selected buzzer
- **AND** send 'W' key to PC
- **AND** remain in partial lockout (or full lockout if last buzzer)
- **AND** continue allowing remaining active buzzers

### Requirement: Full Reset
The game SHALL support manual reset from any state to recover from errors or skip a question.

#### Scenario: Reset from any state
- **WHEN** the reset button is pressed
- **AND** the game is in any state (ready, locked, partial lockout)
- **THEN** the game SHALL transition to ready state
- **AND** send 'R' key to PC
- **AND** unlock all buzzers
- **AND** turn on all buzzer LEDs
- **AND** enable all buzzer buttons
- **AND** clear the selected buzzer

### Requirement: State Persistence
The game state SHALL be maintained in volatile memory and reset on power cycle.

#### Scenario: Power cycle resets state
- **WHEN** the main controller loses power
- **AND** power is restored
- **THEN** the game SHALL initialize to ready state
- **AND** all previous state SHALL be cleared

### Requirement: First-Press Adjudication
The game SHALL accurately determine which buzzer was pressed first when multiple presses occur close in time.

#### Scenario: Clear first press (>50ms gap)
- **WHEN** buzzer A is pressed
- **AND** buzzer B is pressed more than 50ms later
- **THEN** buzzer A SHALL be registered as first
- **AND** buzzer B press SHALL be ignored

#### Scenario: Simultaneous press (<1ms gap)
- **WHEN** multiple buzzers are pressed within 1ms
- **THEN** the buzzer whose message was received first SHALL be registered
- **AND** the tie SHALL be broken by reception timestamp (micros())
- **AND** subsequent presses SHALL be ignored

#### Scenario: Message reordering
- **WHEN** buzzer A presses first but message arrives second (network delay)
- **AND** buzzer B presses second but message arrives first
- **THEN** the message with earliest sender timestamp SHALL be used
- **AND** reception order SHALL be secondary tiebreaker

### Requirement: Locked Buzzer Tracking
The game SHALL track which buzzers are locked out and persist this through state transitions.

#### Scenario: Multiple wrong answers accumulate lockouts
- **WHEN** buzzer 1 gives wrong answer (locked out)
- **AND** buzzer 2 gives wrong answer (locked out)
- **AND** buzzer 3 presses (still active)
- **THEN** only buzzers 3 and 4 SHALL be active
- **AND** buzzers 1 and 2 SHALL remain locked

#### Scenario: Reset clears all lockouts
- **WHEN** multiple buzzers are locked out
- **AND** reset button is pressed
- **THEN** all buzzers SHALL be unlocked
- **AND** all buzzers SHALL become active

