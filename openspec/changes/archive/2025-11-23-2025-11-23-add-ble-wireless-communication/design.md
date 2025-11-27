# Design: BLE Wireless Communication

## Architecture Overview

This design adds a BLE GATT (Generic Attribute Profile) server to the main controller ESP32, enabling wireless communication with PC and smartphone clients while maintaining full compatibility with existing ESP-NOW and USB Serial functionality.

## System Components

```
┌────────────────────────────────────────────────────────────────┐
│                      Main Controller ESP32                      │
│                                                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │  ESP-NOW     │  │  BLE GATT    │  │  USB Serial  │         │
│  │  (WiFi       │  │  Server      │  │              │         │
│  │   Radio)     │  │  (BT Radio)  │  │              │         │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘         │
│         │                  │                  │                  │
│         └──────────────────┼──────────────────┘                 │
│                            │                                     │
│                   ┌────────▼────────┐                           │
│                   │  Game State     │                           │
│                   │  Machine        │                           │
│                   └─────────────────┘                           │
└────────────────────────────────────────────────────────────────┘
         │                   │                  │
         │                   │                  │
    (ESP-NOW)            (BLE)             (USB)
         │                   │                  │
         ▼                   ▼                  ▼
   ┌──────────┐      ┌──────────────┐   ┌──────────┐
   │  Buzzer  │      │   Smartphone │   │    PC    │
   │  Nodes   │      │   (Android/  │   │  (Debug/ │
   │  1-4     │      │     iOS)     │   │  Dev)    │
   └──────────┘      │              │   └──────────┘
                     │      PC      │
                     │  (Windows/   │
                     │   Linux)     │
                     └──────────────┘
```

## BLE GATT Service Design

### Service UUID Structure
- **Quiz Buzzer Service**: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` (Nordic UART Service compatible)

### GATT Characteristics

#### 1. TX Characteristic (Notifications from Controller → Client)
- **UUID**: `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`
- **Properties**: Notify, Read
- **Purpose**: Send game events to connected clients
- **Format**: ASCII text, newline-terminated (same as USB serial)
- **Messages**:
  - `BUZZER:<id>\n` - Buzzer press event
  - `CORRECT\n` - Correct answer button pressed
  - `WRONG\n` - Wrong answer button pressed
  - `RESET\n` - Reset button pressed
  - `DISCONNECT:<id>\n` - Buzzer node disconnected
  - `RECONNECT:<id>\n` - Buzzer node reconnected
  - `STATE_SYNC:<id>\ (...)\n` - State sync debug message

#### 2. RX Characteristic (Write from Client → Controller)
- **UUID**: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`
- **Properties**: Write, Write Without Response
- **Purpose**: Receive control commands from connected clients
- **Format**: ASCII text, newline-terminated (same as USB serial)
- **Commands**:
  - `CORRECT\n` - Execute correct answer action
  - `WRONG\n` - Execute wrong answer action
  - `RESET\n` - Execute reset action

**Why Nordic UART Service UUIDs?**
- Industry-standard BLE UART service
- Widely supported by BLE libraries and terminal apps
- Simple text-based protocol (human-readable)
- Compatible with existing serial protocol
- Easy to test with off-the-shelf apps (nRF UART, Serial Bluetooth Terminal)

## Technical Implementation Details

### BLE Stack

**Library**: ESP32 BLE Arduino (built-in with ESP32 Arduino framework)
- `BLEDevice` - Initialize BLE
- `BLEServer` - Create GATT server
- `BLEService` - Define Quiz Buzzer service
- `BLECharacteristic` - Define TX/RX characteristics
- `BLEAdvertising` - Advertise service

### Initialization Sequence

```cpp
void initBLE() {
  // 1. Initialize BLE device with friendly name
  BLEDevice::init("QuizBuzzer-XXXX"); // XXXX = last 4 digits of MAC
  
  // 2. Create BLE server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  // 3. Create Quiz Buzzer service (Nordic UART Service UUID)
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // 4. Create TX characteristic (notifications)
  pTxCharacteristic = pService->createCharacteristic(
    TX_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
  );
  pTxCharacteristic->addDescriptor(new BLE2902()); // Enable notifications
  
  // 5. Create RX characteristic (writes)
  pRxCharacteristic = pService->createCharacteristic(
    RX_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  pRxCharacteristic->setCallbacks(new RxCallbacks());
  
  // 6. Start service
  pService->start();
  
  // 7. Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // Connection interval
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();
}
```

### Message Bridging Strategy

All game events will be sent to **both** USB Serial and BLE simultaneously:

```cpp
void sendToAllInterfaces(const String& message) {
  // Send to USB Serial (always)
  Serial.println(message);
  
  // Send to BLE if client connected
  if (bleClientConnected && pTxCharacteristic != nullptr) {
    pTxCharacteristic->setValue(message.c_str());
    pTxCharacteristic->notify();
  }
}
```

**Key Points:**
- USB Serial always works (for debugging)
- BLE only sends when client is connected
- No queueing needed for BLE (notifications are fire-and-forget)
- Message format identical across both transports

### Command Processing

Commands from BLE RX characteristic will use the same processing path as USB Serial commands:

```cpp
class RxCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    
    if (value.length() > 0) {
      String command = String(value.c_str());
      command.trim(); // Remove whitespace and newlines
      
      // Reuse existing serial command handler
      processCommand(command);
    }
  }
};
```

### Connection Management

```cpp
class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    bleClientConnected = true;
    Serial.println("BLE client connected");
  }

  void onDisconnect(BLEServer* pServer) {
    bleClientConnected = false;
    Serial.println("BLE client disconnected");
    
    // Restart advertising for new connections
    BLEDevice::startAdvertising();
  }
};
```

## Memory Considerations

### BLE Stack Overhead
- **Estimated RAM usage**: ~40-50KB for BLE stack
- **Available RAM on ESP32**: ~320KB total
- **Current usage**: ~80-100KB (estimated)
- **Headroom**: ~170-200KB remaining - **SUFFICIENT**

### Mitigation Strategies
1. Monitor heap usage during development: `ESP.getFreeHeap()`
2. Test with all systems active (ESP-NOW + BLE + Serial)
3. Reduce BLE MTU if needed (default 517 bytes, can reduce to 23 bytes minimum)
4. Limit to 1 concurrent BLE connection

## Performance Considerations

### Latency Expectations

| Transport | Typical Latency | Max Latency | Notes |
|-----------|----------------|-------------|-------|
| ESP-NOW | 10-30ms | 50ms | Existing, baseline |
| USB Serial | 5-10ms | 20ms | Existing, lowest latency |
| BLE Notify | 30-80ms | 150ms | New, depends on connection interval |
| BLE Write | 30-80ms | 150ms | New, depends on connection interval |

**Connection Interval Impact:**
- Minimum: 7.5ms (0x06) - Lower latency, higher power
- Maximum: 22.5ms (0x12) - Balance latency and power
- Can be negotiated by client

**Acceptable for Quiz Application:**
- Buzzer press → PC display: 100-200ms total latency acceptable
- Human perception threshold: ~100-150ms
- BLE adds ~50-100ms, still within acceptable range

### Throughput
- **Message rate**: <10 messages/second typical for quiz application
- **BLE capacity**: ~100+ short messages/second possible
- **Bottleneck**: Human input speed, not BLE throughput

## Radio Coexistence

### ESP32 Dual-Radio Architecture
- **WiFi Radio**: Used by ESP-NOW for buzzer communication
- **Bluetooth Radio**: Separate physical radio for BLE
- **Coexistence Controller**: Hardware-level arbitration ensures both radios work simultaneously

### Known Issues & Mitigations
- **Minor throughput reduction**: 5-10% when both radios active - **ACCEPTABLE**
- **No packet loss expected**: Coexistence controller handles scheduling
- **Tested configuration**: ESP-NOW + BLE is officially supported by Espressif

### Testing Requirements
- Verify ESP-NOW latency unchanged with BLE active
- Verify BLE latency stable during ESP-NOW traffic bursts
- Monitor packet loss on both transports

## Security Considerations

### BLE Security Model
- **Phase 1 (this proposal)**: No pairing, no encryption
  - Acceptable for short-range quiz application
  - Physical proximity required (~10-30m)
  - No sensitive data transmitted
  
- **Phase 2 (future)**: Optional pairing with PIN
  - Add PIN-based pairing if needed
  - Use BLE Security Manager Protocol (SMP)
  - Not required for initial deployment

### Attack Surface
- **Denial of Service**: Client can connect and block others
  - Mitigation: Reset via physical button
  - Mitigation: Auto-disconnect idle clients after 5 minutes
  
- **Command Injection**: Malicious client sends bogus commands
  - Mitigation: Same as USB serial (physical access required)
  - Mitigation: Commands validated before execution

## Client Implementation Guidance

### Platform Support

#### Android
- **API**: Android Bluetooth LE API (API 18+)
- **Libraries**: No additional libraries needed (native)
- **Example**: Android BLE Scanner app, nRF UART app

#### iOS
- **API**: CoreBluetooth framework (iOS 10+)
- **Libraries**: No additional libraries needed (native)
- **Example**: LightBlue app, nRF UART app

#### Windows
- **API**: Windows Bluetooth LE API (Windows 10+)
- **Libraries**: `Windows.Devices.Bluetooth` (UWP), `bleak` (Python)
- **Fallback**: Web Bluetooth (Chrome browser)

#### Linux
- **API**: BlueZ D-Bus API
- **Libraries**: `bluepy` (Python), `bleak` (Python)
- **Fallback**: Web Bluetooth (Chrome browser)

### Web Bluetooth Option
- **Cross-platform**: Works on Chrome/Edge (Windows, Linux, Android)
- **No installation**: Browser-based, instant use
- **Limitations**: Requires HTTPS, not available on iOS Safari
- **Use case**: Quick testing, demos, simple web dashboard

## Compatibility Matrix

| Feature | ESP-NOW | USB Serial | BLE |
|---------|---------|------------|-----|
| Buzzer nodes | ✓ | - | - |
| Game events to PC | - | ✓ | ✓ |
| Control commands from PC | - | ✓ | ✓ |
| Debug logging | - | ✓ | ✓ |
| Smartphone support | - | ✗ | ✓ |
| Wireless | ✓ | ✗ | ✓ |
| Low latency | ✓ | ✓ | ~ |
| No pairing needed | ✓ | ✓ | ✓ |
| Works simultaneously | ✓ | ✓ | ✓ |

## Alternative Designs Considered

### 1. WiFi AP + WebSocket
**Rejected** - Requires clients to disconnect from internet WiFi, conflicts with ESP-NOW's WIFI_STA usage.

### 2. WiFi STA + WebSocket
**Rejected** - Conflicts with ESP-NOW's custom MAC addressing and WIFI_STA mode requirements.

### 3. Bluetooth Classic SPP
**Rejected** - Poor iOS support, higher latency than BLE, higher power consumption.

### 4. Custom BLE service UUIDs
**Rejected** - Nordic UART Service is industry standard, better compatibility with existing tools and apps.

## Migration Path

### Phase 1: Core BLE Implementation (This Proposal)
- BLE GATT server with TX/RX characteristics
- Message bridging to USB Serial
- Command processing from BLE
- Basic connection management
- No pairing/encryption

### Phase 2: Enhanced Features (Future)
- Optional PIN-based pairing
- BLE connection status LED
- Configurable device name
- Client presence timeout
- Battery level characteristic (for future battery operation)

### Phase 3: Reference Clients (Future)
- Android app (Kotlin)
- Web Bluetooth dashboard (HTML/JS)
- Python CLI tool for testing
- Documentation for custom client development

## Success Metrics

- BLE initialization succeeds without affecting ESP-NOW operation
- Game events delivered via BLE with <100ms latency (95th percentile)
- Control commands processed via BLE within 50ms
- USB Serial continues to function normally when BLE active
- Memory usage remains below 80% of available RAM
- No increase in ESP-NOW packet loss or latency
- Android client can connect and control quiz system
- System remains stable over 1-hour test session
