# USB Serial Protocol

## Connection Settings
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Flow Control**: None

## Message Format
All messages are newline-terminated ASCII text strings (`\n`).

## Protocol Messages

### Buzzer Press Events
```
BUZZER:1\n    # Player 1 buzzed in
BUZZER:2\n    # Player 2 buzzed in
BUZZER:3\n    # Player 3 buzzed in
BUZZER:4\n    # Player 4 buzzed in
```

### Control Button Events
```
CORRECT\n     # Correct answer button pressed
WRONG\n       # Wrong answer button pressed
RESET\n       # Reset button pressed
```

## Debug Messages
Debug messages are prefixed with `DEBUG: ` and can be filtered out by quiz software:
```
DEBUG: System initialized\n
DEBUG: ESP-NOW ready\n
DEBUG: ERROR: Communication timeout\n
```

## Example Session
```
DEBUG: Quiz Buzzer System v1.0\n
DEBUG: Initializing ESP-NOW...\n
DEBUG: ESP-NOW ready\n
DEBUG: Ready for input\n
BUZZER:3\n
WRONG\n
BUZZER:1\n
CORRECT\n
RESET\n
```

## PC Software Integration

### Python Example
```python
import serial

ser = serial.Serial('/dev/ttyUSB0', 115200)

while True:
    line = ser.readline().decode('utf-8').strip()
    
    if line.startswith('DEBUG:'):
        print(f"[DEBUG] {line[7:]}")  # Optional: log debug info
        continue
    
    if line.startswith('BUZZER:'):
        buzzer_id = int(line.split(':')[1])
        print(f"Player {buzzer_id} buzzed in!")
    elif line == 'CORRECT':
        print("Correct answer!")
    elif line == 'WRONG':
        print("Wrong answer!")
    elif line == 'RESET':
        print("Game reset!")
```

### Node.js Example
```javascript
const SerialPort = require('serialport');
const Readline = require('@serialport/parser-readline');

const port = new SerialPort('COM3', { baudRate: 115200 });
const parser = port.pipe(new Readline({ delimiter: '\n' }));

parser.on('data', (line) => {
  if (line.startsWith('DEBUG:')) {
    console.log(`[DEBUG] ${line.substring(7)}`);
    return;
  }
  
  if (line.startsWith('BUZZER:')) {
    const buzzerId = parseInt(line.split(':')[1]);
    console.log(`Player ${buzzerId} buzzed in!`);
  } else if (line === 'CORRECT') {
    console.log('Correct answer!');
  } else if (line === 'WRONG') {
    console.log('Wrong answer!');
  } else if (line === 'RESET') {
    console.log('Game reset!');
  }
});
```

## Message Timing
- Messages sent within **50ms** of event occurrence
- Messages arrive at PC within **100ms** of button press
- Queue holds up to **10 messages** during bursts
- Oldest messages discarded if queue overflows

## Error Handling
- System continues operating if USB disconnected
- Messages queued (up to 10) and sent when reconnected
- Debug messages indicate errors and status changes
