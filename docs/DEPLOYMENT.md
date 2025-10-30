# Deployment Guide

## Hardware Setup

### Components Needed
- 5x Lolin32 Lite boards (ESP32)
- 7x Push buttons (4 for buzzers, 3 for control)
- 4x LEDs (any color)
- 4x Resistors (220Ω for LEDs)
- 4x 1100mAh 3.7V LiPo batteries (for buzzer nodes)
- 1x USB cable (for main controller)
- Breadboards or custom PCB
- Jumper wires

### Wiring

#### Buzzer Nodes (4 boards)
```
ESP32 Pin 12 → Button → GND (internal pullup enabled)
ESP32 Pin 13 → LED → 220Ω Resistor → GND
ESP32 3.3V   → [future speaker connection]
ESP32 GND    → Common ground
Battery +    → ESP32 VIN or USB
Battery -    → ESP32 GND
```

#### Main Controller (1 board)
```
ESP32 Pin 25 → Correct Button → GND (internal pullup)
ESP32 Pin 26 → Wrong Button → GND (internal pullup)
ESP32 Pin 27 → Reset Button → GND (internal pullup)
ESP32 USB    → PC USB port
```

## Software Flashing

### Prerequisites
1. Install [PlatformIO](https://platformio.org/install) (or use PlatformIO IDE extension for VS Code)
2. Install USB drivers for Lolin32 Lite (CH340 or CP2102)
3. Clone this repository

### Build and Flash Commands

#### Flash Main Controller
```bash
# Connect main controller board via USB
pio run -e main_controller -t upload

# Monitor serial output (optional)
pio device monitor -e main_controller
```

#### Flash Buzzer Nodes (one at a time)
```bash
# Flash buzzer node 1
pio run -e buzzer_node_1 -t upload

# Flash buzzer node 2
pio run -e buzzer_node_2 -t upload

# Flash buzzer node 3
pio run -e buzzer_node_3 -t upload

# Flash buzzer node 4
pio run -e buzzer_node_4 -t upload
```

### Physical Labeling
After flashing each board, **immediately label it** with a sticker or marker:
- Main controller: **"MAIN"**
- Buzzer nodes: **"Buzzer 1"**, **"Buzzer 2"**, **"Buzzer 3"**, **"Buzzer 4"**

This prevents confusion during deployment.

## Verification

### 1. Check Serial Output (Main Controller)
```bash
pio device monitor -e main_controller
```

Expected output:
```
========================================
MAIN CONTROLLER
========================================
Custom MAC address: AA:BB:CC:DD:EE:00
✓ Custom MAC address set successfully
✓ ESP-NOW initialized
✓ Buzzer 1 added as peer
✓ Buzzer 2 added as peer
✓ Buzzer 3 added as peer
✓ Buzzer 4 added as peer
========================================
Main controller ready!
Initializing all LEDs to ON (READY state)
========================================
```

### 2. Check Serial Output (Buzzer Nodes)
For each buzzer node:
```bash
pio device monitor -e buzzer_node_1  # or 2, 3, 4
```

Expected output:
```
========================================
BUZZER NODE 1
========================================
Custom MAC address: AA:BB:CC:DD:EE:01
✓ Custom MAC address set successfully
✓ ESP-NOW initialized
✓ Main controller added as peer
========================================
Buzzer node ready!
========================================
```

### 3. Test Communication
1. Power up main controller first
2. Power up all buzzer nodes
3. All buzzer LEDs should turn ON (ready state)
4. Press any buzzer button
5. That buzzer's LED should start blinking
6. All other buzzer LEDs should turn OFF
7. Main controller serial should show: `BUZZER:X` (where X is 1-4)

### 4. Test Control Buttons
1. Press CORRECT button → All LEDs turn ON, serial shows `CORRECT`
2. Press buzzer again → That buzzer blinks
3. Press WRONG button → Pressed buzzer OFF, others ON, serial shows `WRONG`
4. Press another buzzer → That buzzer blinks
5. Press RESET button → All LEDs ON, serial shows `RESET`

## PC Integration

### Find Serial Port
**Linux/Mac:**
```bash
ls /dev/tty* | grep USB
# Usually /dev/ttyUSB0 or /dev/cu.usbserial-*
```

**Windows:**
```
Device Manager → Ports (COM & LPT)
# Usually COM3, COM4, etc.
```

### Test with Terminal
**Linux/Mac:**
```bash
screen /dev/ttyUSB0 115200
# Or
cat /dev/ttyUSB0
```

**Windows:**
```
# Use PuTTY or Arduino Serial Monitor
# Set baud rate to 115200
```

### Integrate with Quiz Software
See `PROTOCOLS.md` for Python/Node.js examples.

## Troubleshooting

### Buzzer Node LED Not Responding
- Check ESP-NOW initialization (look for errors in serial output)
- Verify custom MAC addresses are set (should show AA:BB:CC:DD:EE:0X)
- Ensure main controller is powered on first
- Check that buzzer node was added as peer on main controller

### Button Press Not Detected
- Test button with multimeter (should short to GND when pressed)
- Check GPIO pin assignments match hardware wiring
- Verify internal pullup is enabled (default in code)
- Check debounce timing (50ms default)

### Serial Messages Not Appearing
- Verify baud rate is 115200
- Check USB cable and drivers
- Look for serial port permissions (Linux: add user to `dialout` group)
- Ensure main controller is receiving button press events (check local serial output)

### ESP-NOW Communication Failures
- Reduce distance between boards (try < 10m initially)
- Check for WiFi interference (try different channel in config.h)
- Verify all boards have correct custom MAC addresses
- Ensure all boards are using same ESP-NOW channel

### Multiple Definition Errors During Build
- This usually means build_src_filter is not working correctly
- Verify platformio.ini has `-<*>` to exclude all files by default
- Check that only the correct .cpp file is included per environment

## Battery Management (Buzzer Nodes)

### Battery Life
- Expected: 4-6 hours continuous use
- Light sleep mode reduces power consumption to ~8mA
- Active mode (LED on): ~100mA

### Low Battery Indication
Currently not implemented in v1. Consider adding voltage monitoring if needed.

### Charging
- Disconnect battery before charging (or use board with built-in charging circuit)
- Use appropriate LiPo charger (3.7V, 1C = 1.1A max)
- Never leave charging unattended

## Production Deployment Checklist

- [ ] All 5 boards flashed with correct firmware
- [ ] All boards physically labeled (Buzzer 1-4, MAIN)
- [ ] All buzzer nodes tested individually
- [ ] Main controller receives messages from all buzzers
- [ ] Control buttons (CORRECT/WRONG/RESET) tested
- [ ] PC serial communication verified
- [ ] Battery life tested (if using batteries)
- [ ] Backup boards prepared (recommended)
- [ ] Quiz software integration tested
- [ ] Range test performed (ensure adequate signal strength)

## Future Enhancements

### Adding Speakers (v2)
- GPIO 14 is reserved on buzzer nodes
- Consider passive buzzers or small speakers
- Add tone() calls in buzzer_node.cpp on button press and LED state changes
- See future proposal for speaker integration

### Battery Power Optimization
- Current implementation uses light sleep between button presses
- Deep sleep not recommended (loses ESP-NOW connection, slow wake)
- Consider adding low-battery voltage monitoring (ADC pin to battery divider)

### Over-the-Air Updates
- Not currently implemented
- Would require WiFi connection and OTA library
- Consider for future versions if remote deployment needed
