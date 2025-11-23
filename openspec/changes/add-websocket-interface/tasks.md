# Implementation Tasks

## 1. Dependencies and Configuration
- [x] 1.1 Add `links2004/arduinoWebSockets` library to platformio.ini
- [x] 1.2 Add `ESPAsyncWebServer` library to platformio.ini (for config portal)
- [x] 1.3 Add `DNSServer` library to platformio.ini (for captive portal)
- [x] 1.4 Add WiFi configuration constants to src/config.h (AP SSID, setup SSID, port, timeout values)
- [x] 1.5 Add WebSocket configuration constants (max clients, ping interval, buffer size)
- [x] 1.6 Add build flag `-DDISABLE_WEBSOCKET` option for conditional compilation
- [x] 1.7 Add mDNS hostname constant ("quizbuzzer")

## 2. WiFi Credentials Storage (NVS)
- [x] 2.1 Include Preferences.h library for NVS access
- [x] 2.2 Implement loadWiFiCredentials() function (read from NVS)
- [x] 2.3 Implement saveWiFiCredentials(ssid, password) function (write to NVS)
- [x] 2.4 Implement clearWiFiCredentials() function (erase from NVS)
- [x] 2.5 Add hasWiFiCredentials() check function (returns bool)
- [x] 2.6 Test NVS persistence across power cycles

## 3. WiFi Configuration Web Interface
- [x] 3.1 Create HTML configuration page (embedded in program memory as const char*)
- [x] 3.2 Implement WiFi network scanner endpoint (GET /scan)
- [x] 3.3 Implement credential save endpoint (POST /save with SSID and password)
- [x] 3.4 Implement current status endpoint (GET /status)
- [x] 3.5 Implement WiFi reset endpoint (POST /reset)
- [x] 3.6 Add JavaScript for dynamic network list and form submission
- [x] 3.7 Style web interface with responsive CSS (mobile-friendly)
- [x] 3.8 Add loading indicators and error messages

## 4. Captive Portal Implementation
- [x] 4.1 Instantiate DNSServer object for AP Setup Mode
- [x] 4.2 Configure DNS to redirect all requests to 192.168.4.1
- [x] 4.3 Instantiate AsyncWebServer on port 80
- [x] 4.4 Register web interface endpoints with AsyncWebServer
- [x] 4.5 Call dnsServer.processNextRequest() in main loop during AP Setup Mode
- [ ] 4.6 Test captive portal redirect on iOS and Android devices

## 5. WiFi Mode State Machine
- [x] 5.1 Define WiFi mode enum (SETUP_MODE, STATION_MODE, CONNECTING)
- [x] 5.2 Implement checkWiFiResetButton() function (check GPIO 27 held for 3 seconds during boot)
- [x] 5.3 Implement enterSetupMode() function:
  - Start WiFi AP with "QuizBuzzer-Setup" SSID (open network)
  - Start DNS server for captive portal
  - Start HTTP server for config web interface
  - Set mode to SETUP_MODE
- [x] 5.4 Implement enterStationMode() function:
  - Load credentials from NVS
  - Connect to WiFi network
  - Wait up to 30 seconds for connection
  - If success: start mDNS, proceed to connected state
  - If failure: fallback to enterSetupMode()
- [x] 5.5 Implement boot logic in setup():
  - Check if RESET button (GPIO 27) held for 3 seconds during boot
  - If yes: clearWiFiCredentials() and enterSetupMode()
  - Else if hasWiFiCredentials(): enterStationMode()
  - Else: enterSetupMode()
  - Note: This check occurs before game state initialization
- [x] 5.6 Implement WiFi connection monitoring in loop():
  - Periodically check WiFi.status()
  - If disconnected for >30s in STATION_MODE: enterSetupMode()

## 6. mDNS Service Announcement
- [x] 6.1 Include ESP32 mDNS library
- [x] 6.2 Initialize mDNS with hostname "quizbuzzer" when in STATION_MODE
- [x] 6.3 Advertise WebSocket service on port 8080 (_ws._tcp)
- [x] 6.4 Respond to mDNS queries in main loop
- [ ] 6.5 Test resolution of quizbuzzer.local from various devices

## 7. WiFi Access Point Setup Mode
- [x] 7.1 Implement WiFi AP initialization for setup mode
- [x] 7.2 Configure SSID "QuizBuzzer-Setup" (open network, no password)
- [x] 7.3 Set static IP 192.168.4.1, gateway, subnet mask
- [x] 7.4 Configure WiFi channel 1 (shared with ESP-NOW)
- [x] 7.5 Enable DHCP server (assign IPs 192.168.4.2-192.168.4.10)
- [x] 7.6 Add LED indicator for setup mode (rapid blink pattern)
- [x] 7.7 Log "Setup Mode Active" to USB serial with instructions

## 8. WiFi Station Mode Implementation
- [x] 8.1 Implement WiFi Station initialization
- [x] 8.2 Call WiFi.begin(ssid, password) with stored credentials
- [x] 8.3 Wait for connection with timeout (30 seconds)
- [x] 8.4 Log connection status to serial (success with IP, or failure)
- [x] 8.5 Synchronize ESP-NOW channel with WiFi router channel
- [ ] 8.6 Verify ESP-NOW continues to function in Station Mode
- [x] 8.7 Display "WiFi Connected: <SSID> | IP: <address> | mDNS: quizbuzzer.local" on serial

## 9. WebSocket Server Setup
- [x] 9.1 Instantiate WebSocketsServer object on port 8080
- [x] 9.2 Implement WebSocket event handler callback function (connection, disconnection, text message)
- [x] 9.3 Call webSocketServer.loop() in main loop() function
- [x] 9.4 Implement client connection tracking (store client IDs in array)
- [x] 9.5 Send state sync message immediately upon client connection
- [x] 9.6 Log WebSocket events to USB serial (DEBUG: WS: prefix)

## 10. JSON Message Formatting
- [x] 10.1 Implement JSON formatting functions for each message type:
  - formatBuzzerJson(uint8_t id)
  - formatCorrectJson()
  - formatWrongJson()
  - formatResetJson()
  - formatDisconnectJson(uint8_t id)
  - formatReconnectJson(uint8_t id)
  - formatStateJson(uint8_t selected, uint8_t lockedBitmask)
- [x] 10.2 Add timestamp field (millis()) to all JSON messages
- [x] 10.3 Test JSON output format validity (ensure proper escaping, valid structure)

## 11. Unified Message Broadcasting
- [x] 11.1 Create broadcastMessage() abstraction function
- [x] 11.2 Refactor existing serial message calls to use broadcastMessage()
- [x] 11.3 Implement WebSocket broadcast to all connected clients in broadcastMessage()
- [x] 11.4 Ensure USB serial format remains unchanged (backward compatibility)
- [x] 11.5 Add error handling for WebSocket send failures (log and continue)

## 12. JSON Command Processing
- [x] 12.1 Implement WebSocket text message handler in event callback
- [x] 12.2 Parse incoming JSON (extract "command" field)
- [x] 12.3 Route commands to existing handler functions (handleCorrectAnswer, handleWrongAnswer, handleReset)
- [x] 12.4 Implement getState command handler (query current game state)
- [x] 12.5 Send JSON acknowledgment response to commanding client
- [x] 12.6 Handle invalid JSON and missing fields (send error response)
- [x] 12.7 Verify commands from WebSocket trigger USB serial messages (dual broadcast)

## 13. Connection Management
- [x] 13.1 Implement WebSocket ping/pong heartbeat (send ping every 5 seconds)
- [x] 13.2 Detect client timeout (no pong response within 2 seconds)
- [x] 13.3 Clean up disconnected clients from tracking array
- [x] 13.4 Implement per-client message buffer (max 10 messages)
- [x] 13.5 Drop oldest messages if buffer exceeds limit (log overflow)

## 14. Testing and Validation
- [ ] 14.1 Test first boot into AP Setup Mode (unconfigured)
- [ ] 14.2 Test captive portal redirect on iOS device
- [ ] 14.3 Test captive portal redirect on Android device
- [ ] 14.4 Test WiFi network scanning in web interface
- [ ] 14.5 Test saving valid WiFi credentials and automatic reboot
- [ ] 14.6 Test Station Mode connection to home WiFi network
- [ ] 14.7 Test mDNS resolution (quizbuzzer.local) from multiple devices
- [ ] 14.8 Test WiFi reset button hold during boot (force setup mode)
- [ ] 14.9 Test automatic fallback to Setup Mode on connection failure
- [ ] 14.10 Test NVS persistence (credentials survive power cycle)
- [ ] 14.11 Test WiFi AP discovery from phone/tablet/laptop in Setup Mode
- [ ] 14.12 Test WebSocket connection from browser (simple HTML test page)
- [ ] 14.13 Verify ESP-NOW buzzer communication remains <30ms latency with WiFi active (both modes)
- [ ] 14.14 Test concurrent USB serial + WebSocket operation (no interference)
- [ ] 14.15 Test multiple simultaneous WebSocket clients (up to 4 connections)
- [ ] 14.16 Verify all game events broadcast to both interfaces
- [ ] 14.17 Test commands from WebSocket trigger correct game state transitions
- [ ] 14.18 Test automatic state sync on new WebSocket connection
- [ ] 14.19 Load test with rapid events (ensure no message loss or blocking)
- [ ] 14.20 Test reconnection after client disconnection (graceful and timeout)
- [ ] 14.21 Test WebSocket connection in Station Mode with internet access maintained
- [ ] 14.22 Test ESP-NOW channel synchronization when router changes channels

## 15. Documentation
- [x] 15.1 Update README.md with first-time setup instructions (captive portal flow)
- [x] 15.2 Document WiFi reset procedure (button hold during boot)
- [x] 15.3 Add WebSocket connection instructions for both Station and Setup modes
- [x] 15.4 Add JSON message format reference to docs/PROTOCOLS.md
- [x] 15.5 Document mDNS usage (quizbuzzer.local)
- [x] 15.6 Add Godot integration example (GDScript code snippet with auto-reconnect)
- [x] 15.7 Create simple HTML test page for browser-based testing
- [x] 15.8 Update troubleshooting section with WiFi and WebSocket issues
- [x] 15.9 Document NVS storage usage and manual reset options

## 16. Optional Enhancements (Future)
- [ ] 16.1 Add OTA (Over-The-Air) firmware update via web interface
- [ ] 16.2 Implement client authentication (bearer token or session ID)
- [ ] 16.3 Add WebSocket Secure (WSS) support with self-signed certificate
- [ ] 16.4 Create complete Godot reference project (mobile quiz app)
- [ ] 16.5 Add WiFi signal strength indicator in web interface
- [ ] 16.6 Implement automatic WiFi channel switching to avoid interference
- [ ] 16.7 Add support for static IP configuration (optional alternative to DHCP)
