## Why
This project needs a fully functional multi-board quiz buzzer system with 4 independent buzzer nodes communicating wirelessly with a main controller board. The system must handle first-press detection, visual feedback, answer validation workflow, and PC integration for quiz game management.

## What Changes
- Add 4 buzzer node firmware (button input, LED output, ESP-NOW communication)
- Add main controller firmware (ESP-NOW hub, USB serial communication, control buttons)
- Implement distributed first-press detection with lockout mechanism
- Create game state machine (ready, locked, partial-lockout states)
- Add visual feedback system (solid LED = active, blinking = selected, off = locked out)
- Implement three-button control interface (correct answer, wrong answer, full reset)
- Add PC interface via USB serial protocol (buzzer events and control commands as text messages)
- Support answer validation workflow with selective lockout on wrong answers
- Reserve GPIO pins on buzzer nodes for future audio feedback (speakers deferred to future proposal)

## Impact
- Affected specs: buzzer-node, main-controller, game-state, pc-interface (all new)
- Affected code: src/main.cpp (will split into multiple files for different board types)
- Hardware requirements: 5x Lolin32 Lite boards, 4x buttons (buzzer inputs), 7x buttons (total), 4x LEDs, USB cable for main board, 4x 1100mAh 3.7V LiPo batteries (for buzzer nodes)
- Communication: ESP-NOW mesh network between boards
