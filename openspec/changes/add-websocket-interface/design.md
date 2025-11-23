# WebSocket Interface Design

## Context

The system currently uses USB serial (115200 baud) for bidirectional PC communication, which works well for desktop applications but creates barriers for mobile/cross-platform development. The user wants to build a Godot app that can run on iOS/Android/desktop, requiring a wireless and universally-supported protocol.

**Current Architecture:**
- Buzzer Nodes (4x) ↔ Main Controller: ESP-NOW (wireless, low latency, battery efficient)
- Main Controller ↔ PC: USB Serial (wired, text-based protocol)

**Constraint:** Must keep ESP-NOW for buzzer communication (battery life + latency requirements).

## Goals / Non-Goals

**Goals:**
- Enable wireless communication between main controller and client apps
- Support Godot framework on iOS/Android/desktop/web
- Allow multiple simultaneous connections (e.g., game app + scoreboard display)
- Maintain backward compatibility with existing USB serial interface
- Provide JSON-based structured messages for easier parsing
- Support automatic reconnection and connection status monitoring

**Non-Goals:**
- Changing ESP-NOW protocol for buzzer nodes (already optimal)
- Supporting external MQTT broker or cloud services (standalone system)
- Implementing authentication/encryption (local network, low-security environment)
- Replacing USB serial (both interfaces will coexist)

## Decisions

### Decision 1: WiFi Mode Selection (AP vs Station)

**Choice:** Support **both** WiFi Access Point (AP) mode and Station (STA) mode with runtime configuration (no reflashing required)

**Implementation: Captive Portal Configuration**

The system uses a "smart device" setup pattern:

**First Boot / Unconfigured State:**
1. ESP32 starts in **AP Setup Mode**: `"QuizBuzzer-Setup"` network (no password)
2. User connects with phone/laptop
3. **Captive portal** redirects to configuration web page at `192.168.4.1`
4. Web interface provides:
   - WiFi network scanner (lists available SSIDs)
   - Password input field
   - "Save & Connect" button
5. ESP32 stores credentials in **NVS flash** (persistent, survives reboots)
6. ESP32 reboots into **Station Mode**

**Normal Operation (After Configuration):**
- ESP32 joins configured WiFi network
- Announces presence via **mDNS**: `quizbuzzer.local`
- Displays IP address on USB serial console
- WebSocket available at `ws://quizbuzzer.local:8080` or `ws://<ip>:8080`

**Fallback & Reconfiguration:**
- If WiFi connection fails for 30 seconds → automatic fallback to AP Setup Mode
- **Reset WiFi**: Hold physical button during boot for 3 seconds → force AP Setup Mode
- **Web UI always available** (in both modes) for WiFi reconfiguration

**Why This Approach:**
- ✅ **No reflashing** - WiFi credentials stored in ESP32 NVS (non-volatile storage)
- ✅ **User-friendly** - Same setup flow as smart home devices
- ✅ **Robust** - Automatic fallback if WiFi unavailable
- ✅ **Flexible** - Can reconfigure for different venues/networks
- ✅ **Secure** - Credentials never exposed in source code

**Station Mode (Primary Mode After Setup):**
Main controller joins an existing WiFi network.

**Rationale:**
- **Maintains internet access**: Devices stay connected to home/venue WiFi
- **Multi-device friendly**: Quiz app + other apps/services work simultaneously
- **mDNS discovery**: Can use `quizbuzzer.local` instead of IP address
- **Better UX**: Phone can run quiz app while staying online for cloud quiz data

**Trade-offs:**
- Requires one-time setup process (5 minutes first boot)
- Dynamic IP address (mitigated by mDNS)
- Potential ESP-NOW channel conflicts (must match router channel)
- Requires router present at venue

**AP Setup Mode (Configuration & Fallback):**
Main controller creates its own WiFi Access Point for configuration.

**Rationale:**
- **Initial setup**: Provides web interface for WiFi configuration
- **Fallback mode**: If Station connection fails, returns to setup mode
- **Standalone operation**: Can still use system without home WiFi (if internet not needed)
- **Predictable networking**: Fixed IP address (192.168.4.1)

**Configuration (AP Setup Mode):**
```cpp
SSID: "QuizBuzzer-Setup"
Password: none (open network for first-time setup)
IP: 192.168.4.1
HTTP Server: Port 80 (captive portal)
WebSocket: Port 8080 (still functional in AP mode)
Channel: 1 (shared with ESP-NOW)
Max clients: 4
```

**Configuration (Station Mode):**
```cpp
SSID: <user-configured via web UI>
Password: <user-configured via web UI>
IP: <DHCP assigned>
mDNS: quizbuzzer.local
WebSocket: Port 8080
Channel: <auto-match router channel>
```

**Implementation Details:**
- Use **Preferences library** (ESP32 NVS wrapper) for credential storage
- **DNSServer** for captive portal redirect
- **AsyncWebServer** for configuration web interface
- **WiFiMulti** for connection management and automatic retry

**Alternative Solutions Considered:**
- Bluetooth: Lower throughput, complex pairing, limited multi-device support
- WiFi Direct: Not well-supported on all platforms (especially iOS)
- Hybrid AP+STA: ESP32 supports this, but significantly increases complexity and memory usage

### Decision 2: WebSocket Protocol Library

**Choice:** Use `arduinoWebSockets` library by Links2004

**Rationale:**
- Mature, well-tested ESP32 support
- Low memory footprint (~30-40KB flash)
- Non-blocking async operation
- Handles WebSocket handshake and framing automatically
- Compatible with Godot's built-in WebSocket client

**Alternatives considered:**
- ESPAsyncWebServer + AsyncWebSocket: Heavier, more complex dependencies
- Raw TCP sockets: Would need to implement WebSocket protocol manually
- HTTP REST API: No push notifications, requires polling (latency)

**Library:** https://github.com/Links2004/arduinoWebSockets

### Decision 3: Message Format

**Choice:** JSON for WebSocket, existing text format for USB serial

**WebSocket Message Format:**
```json
// Outbound (Controller → Client)
{"type": "buzzer", "id": 1, "timestamp": 1234567}
{"type": "correct", "timestamp": 1234567}
{"type": "wrong", "timestamp": 1234567}
{"type": "reset", "timestamp": 1234567}
{"type": "disconnect", "id": 2, "timestamp": 1234567}
{"type": "reconnect", "id": 3, "timestamp": 1234567}
{"type": "state", "selected": 1, "locked": [2, 3], "timestamp": 1234567}

// Inbound (Client → Controller)
{"command": "correct"}
{"command": "wrong"}
{"command": "reset"}
{"command": "getState"}  // Request current game state
```

**Rationale:**
- JSON is native in Godot/GDScript (easy parsing)
- Type-safe message structure (vs. string parsing)
- Extensible (can add fields without breaking compatibility)
- Human-readable for debugging

**USB Serial Format (unchanged):**
```
BUZZER:1\n
CORRECT\n
CMD_ACK:CORRECT\n
```

### Decision 4: Dual Interface Architecture

**Choice:** Both USB serial and WebSocket run concurrently, sharing game state

**Implementation:**
```cpp
// Unified message broadcasting
void broadcastMessage(MessageType type, uint8_t value) {
  // Send to USB serial (existing)
  queueSerialMessage(formatSerialMessage(type, value));
  
  // Send to all WebSocket clients (new)
  broadcastWebSocketMessage(formatJsonMessage(type, value));
}
```

**Rationale:**
- Zero breaking changes to existing serial interface
- Supports hybrid setups (USB for debug, WebSocket for gameplay)
- Minimal code duplication via abstraction
- Each interface can fail independently

### Decision 5: Connection Management

**Choice:** Track WebSocket clients, send state sync on connection

**Flow:**
```
1. Client connects to ws://192.168.4.1:8080
2. Controller sends full state sync:
   {"type": "state", "selected": 1, "locked": [2], "timestamp": 123}
3. Controller broadcasts all events to connected clients
4. Client disconnection detected automatically (WebSocket ping/pong)
```

**Rationale:**
- New clients immediately see current game state
- Automatic reconnection recovery (similar to buzzer node state sync)
- No manual client registration required

### Decision 6: Performance Considerations

**Memory Budget:**
- WiFi AP stack: ~25KB
- WebSocket library: ~30KB
- JSON serialization: ~5KB
- **Total overhead: ~60KB flash, ~10KB RAM**

**ESP32 Lolin32 Lite Resources:**
- Flash: 4MB (current usage ~200KB, plenty available)
- RAM: 320KB (current usage ~50KB, sufficient headroom)

**Latency Analysis:**
- USB Serial: ~10ms
- WebSocket (WiFi): ~20-50ms (acceptable for quiz application)
- ESP-NOW (buzzers): ~10-30ms (unchanged)

**Recommendation:** WebSocket latency is acceptable for quiz gameplay (human reaction time >> 50ms).

## Risks / Trade-offs

### Risk: WiFi Channel Conflict with ESP-NOW
- **Mitigation**: Use same channel (1) for both WiFi AP and ESP-NOW
- **Tested compatibility**: ESP32 supports simultaneous ESP-NOW + WiFi SoftAP on same channel

### Risk: Increased Power Consumption
- **Impact**: Minimal (main controller is USB-powered, not battery)
- **Buzzer nodes**: Unaffected (continue using low-power ESP-NOW)

### Risk: WebSocket Connection Stability
- **Mitigation**: Implement heartbeat ping/pong (every 5s)
- **Client-side**: Godot apps should implement auto-reconnect logic
- **Fallback**: USB serial remains available as backup

### Risk: Multiple Client Conflicts
- **Scenario**: Two Godot apps send conflicting commands
- **Mitigation**: Commands are processed FIFO (first-come, first-served)
- **Future enhancement**: Add client priority or coordinator role

### Trade-off: Configuration Complexity
- **Benefit**: Default AP mode works out-of-the-box
- **Cost**: WiFi credentials exposed in source code (acceptable for local quiz system)
- **Alternative**: Future enhancement could add web-based config portal

## Migration Plan

**Phase 1: Implementation (This Change)**
1. Add WiFi AP initialization to controller.cpp
2. Integrate WebSocket server with event loop
3. Implement JSON message formatting
4. Add broadcast mechanism for dual interfaces
5. Update config.h with WiFi constants

**Phase 2: Testing**
1. Verify ESP-NOW + WiFi coexistence (no buzzer lag)
2. Test WebSocket from browser (simple HTML test page)
3. Validate concurrent USB serial + WebSocket operation
4. Load test with multiple connections

**Phase 3: Client Development (Out of Scope)**
1. Godot WebSocket client library integration
2. Mobile app UI development
3. State management and reconnection logic

**Rollback Strategy:**
- WebSocket code is additive (no modifications to existing serial logic)
- Can disable WiFi via build flag if issues arise: `-DDISABLE_WEBSOCKET`
- Existing USB serial workflows unaffected

## Open Questions

1. **WiFi Password Configuration**: Hardcode vs. config file vs. web portal?
   - **Recommendation**: Start with hardcoded (simplest), add config portal later if needed

2. **WebSocket Port**: Use 8080 (standard) or 80 (requires mDNS for friendly access)?
   - **Recommendation**: 8080 (avoid mDNS complexity, explicit IP is fine)

3. **TLS/WSS Support**: Should we support encrypted WebSocket (wss://)?
   - **Recommendation**: Not for v1 (adds complexity, local network is trusted)

4. **Message History Buffer**: Should new WebSocket clients receive recent event history?
   - **Recommendation**: No (just send current state, events are transient)

5. **Godot Integration Example**: Should we provide reference Godot project?
   - **Recommendation**: Out of scope for firmware change, but good for documentation
