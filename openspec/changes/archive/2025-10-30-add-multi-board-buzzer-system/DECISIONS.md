# Design Decisions Summary

## Hardware Configuration

### Speakers
- **Decision**: Each buzzer node will have a speaker (4 speakers total, not on main controller)
- **Status**: DEFERRED to future proposal
- **Current Action**: Reserve GPIO pin on buzzer nodes for future speaker integration
- **Rationale**: Localized audio feedback per player, but not needed for MVP functionality

### Power Supply
- **Decision**: USB power for all 5 boards
- **Future**: Design allows for battery operation on buzzer nodes later
- **Rationale**: Simpler initial deployment, battery can be added without firmware changes

## Software Configuration

### MAC Addresses
- **Decision**: Custom MAC addresses set programmatically
- **Implementation**: 
  ```cpp
  uint8_t customMAC[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, NODE_ID};
  esp_wifi_set_mac(WIFI_IF_STA, customMAC);
  ```
- **Benefits**:
  - No need to read and manually configure MAC addresses
  - Main controller can hardcode known addresses
  - Predictable MAC pattern: :01, :02, :03, :04
  - Same firmware can be flashed to any main controller board

### Node ID Configuration
- **Decision**: Build-time configuration via PlatformIO build flags
- **Implementation**: 4 separate build environments (buzzer_node_1 through buzzer_node_4)
- **Example**: 
  ```ini
  [env:buzzer_node_1]
  build_flags = -DNODE_ID=1
  
  [env:buzzer_node_2]
  build_flags = -DNODE_ID=2
  ```
- **Deployment**: 
  - Flash with: `pio run -e buzzer_node_1 -t upload`
  - Label board physically: "Buzzer 1"

### LED Blink Rate
- **Decision**: 2Hz (500ms on, 500ms off) for selected buzzer
- **Implementation**: `#define BLINK_INTERVAL_MS 500` in config.h
- **Rationale**: Easy to tune based on user feedback

## GPIO Pin Allocation

### Buzzer Node Pins
- Button input: TBD (with internal pullup)
- LED output: TBD
- Speaker (reserved): TBD (not implemented in v1)

### Main Controller Pins
- Control button 1 (correct): TBD (with internal pullup)
- Control button 2 (wrong): TBD (with internal pullup)
- Control button 3 (reset): TBD (with internal pullup)
- USB serial: Built-in (CH340/CP2102)

## Communication Protocols

### ESP-NOW (Buzzer Nodes ↔ Main Controller)
- **Custom MAC addresses**: AA:BB:CC:DD:EE:01 through :04
- **Message format**: Binary struct with node_id, msg_type, value, timestamp
- **Retry logic**: Up to 3 retries with 10ms interval for button presses
- **Latency**: 10-30ms typical

### USB Serial (Main Controller ↔ PC)
- **Baud rate**: 115200
- **Message format**: Newline-terminated ASCII text
- **Protocol**: 
  - `BUZZER:1\n` through `BUZZER:4\n`
  - `CORRECT\n`, `WRONG\n`, `RESET\n`
  - `DEBUG: <message>\n`
- **Latency**: <100ms from button press to PC

## Deployment Process

1. Flash buzzer nodes (4 boards):
   ```bash
   pio run -e buzzer_node_1 -t upload
   pio run -e buzzer_node_2 -t upload
   pio run -e buzzer_node_3 -t upload
   pio run -e buzzer_node_4 -t upload
   ```

2. Label each board physically: "Buzzer 1" through "Buzzer 4"

3. Flash main controller (1 board):
   ```bash
   pio run -e main_controller -t upload
   ```

4. Label main board: "MAIN"

5. Power up and verify custom MAC addresses in serial output

6. Connect main board to PC and test with quiz software

## Future Enhancements (Out of Scope for v1)

- [ ] Speaker audio feedback on buzzer nodes
- [ ] Battery operation for wireless buzzer nodes
- [ ] DIP switch configuration for node IDs
- [ ] Auto-discovery of buzzer nodes
- [ ] Over-the-air (OTA) firmware updates
- [ ] Visual display screen integration
- [ ] Built-in score tracking
