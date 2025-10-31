# Quiz Buzzer System - GPIO Pin Assignments

## Buzzer Node Pins (4 boards)

| Function | GPIO Pin | Mode | Notes |
|----------|----------|------|-------|
| Button Input | 12 | INPUT_PULLUP | Buzzer button, active LOW, white |
| LED Output | 13 | OUTPUT | Status LED (solid/blink/off), red |
| Speaker (reserved) | 14 | Not implemented | Reserved for future audio feedback |

## Main Controller Pins (1 board)

| Function | GPIO Pin | Mode | Notes |
|----------|----------|------|-------|
| Correct Button | 25 | INPUT_PULLUP | Correct answer, active LOW |
| Wrong Button | 26 | INPUT_PULLUP | Wrong answer, active LOW |
| Reset Button | 27 | INPUT_PULLUP | Full reset, active LOW |
| USB Serial | Built-in | - | CH340/CP2102, 115200 baud |

## Hardware Requirements

- 5x Lolin32 Lite boards (ESP32-based)
- 4x Push buttons (for buzzer nodes)
- 3x Push buttons (for main controller)
- 4x LEDs with appropriate resistors (~220Ω for 3.3V)
- 4x 1100mAh 3.7V LiPo batteries (for buzzer nodes)
- 1x USB cable (for main controller → PC connection)
- Breadboards or custom PCB for assembly

## Power Configuration

- **Main Controller**: USB powered (5V from PC)
- **Buzzer Nodes**: Battery powered (1100mAh 3.7V LiPo each)
  - Expected battery life: ~4+ hours with light sleep mode
  - Light sleep mode: ~8mA (vs ~100mA active)
  - Wake on button press via GPIO interrupt

## Notes

- All GPIOs use 3.3V logic levels
- Internal pullups are enabled for all button inputs (no external resistors needed)
- Speaker pins (GPIO 14 on buzzer nodes) are reserved but not used in v1
- Custom MAC addresses are set programmatically, no hardware configuration needed
