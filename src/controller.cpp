#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "protocol.h"
#include "config.h"

// ============================================================================
// GAME STATE MACHINE
// ============================================================================

enum GameState {
  STATE_READY,          // Waiting for first press, all buzzers active
  STATE_LOCKED,         // One buzzer pressed, all others locked out
  STATE_PARTIAL_LOCKOUT // Wrong answer given, that buzzer locked, others can try
};

GameState currentState = STATE_READY;
uint8_t selectedBuzzer = 0;        // 1-4, or 0 if none
uint8_t lockedBuzzers = 0;         // Bitmask: bit 0 = buzzer 1, bit 1 = buzzer 2, etc.
unsigned long lastPressTime = 0;   // For timestamp-based tie breaking

// Known buzzer node MAC addresses (custom MACs set on buzzer nodes)
uint8_t buzzerMACs[NUM_BUZZERS][6] = {
  {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01},
  {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02},
  {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x03},
  {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x04}
};

// Control button state management
bool lastCorrectState = HIGH;
bool lastWrongState = HIGH;
bool lastResetState = HIGH;
unsigned long lastCorrectDebounce = 0;
unsigned long lastWrongDebounce = 0;
unsigned long lastResetDebounce = 0;

// Serial message queue
String messageQueue[MESSAGE_QUEUE_SIZE];
int queueHead = 0;
int queueTail = 0;
int queueCount = 0;

// Connection tracking
unsigned long lastHeartbeatTime = 0;
unsigned long nodeLastSeen[NUM_BUZZERS] = {0, 0, 0, 0};
bool nodeConnected[NUM_BUZZERS] = {false, false, false, false};

// Serial command input
char serialInputBuffer[SERIAL_INPUT_BUFFER_SIZE];
int serialInputIndex = 0;

// BLE variables
BLEServer* pBLEServer = nullptr;
BLECharacteristic* pTxCharacteristic = nullptr;
BLECharacteristic* pRxCharacteristic = nullptr;
bool bleClientConnected = false;
String bleDeviceName = "";

// Forward declarations for BLE callbacks
void handleCorrectAnswer();
void handleWrongAnswer();
void handleFullReset();

// ============================================================================
// BLE CALLBACK CLASSES
// ============================================================================

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
    Serial.println("BLE advertising restarted");
  }
};

class RxCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    
    if (value.length() > 0) {
      String command = String(value.c_str());
      command.trim(); // Remove whitespace and newlines
      
      Serial.print("BLE CMD: ");
      Serial.println(command);
      
      // Process command (reuse serial command logic)
      if (command == "CORRECT") {
        handleCorrectAnswer();
      } else if (command == "WRONG") {
        handleWrongAnswer();
      } else if (command == "RESET") {
        handleFullReset();
      } else if (command.length() > 0) {
        Serial.print("BLE CMD_ERR:UNKNOWN:");
        Serial.println(command);
      }
    }
  }
};

// Forward declarations
void handleCorrectAnswer();
void handleWrongAnswer();
void handleFullReset();

// ============================================================================
// MESSAGE BRIDGING (Send to both Serial and BLE)
// ============================================================================

void sendToAllInterfaces(const String& message) {
  // Always send to USB Serial
  Serial.println(message);
  
  // Send to BLE if client connected
  if (bleClientConnected && pTxCharacteristic != nullptr) {
    String bleMessage = message + "\n";
    pTxCharacteristic->setValue(bleMessage.c_str());
    pTxCharacteristic->notify();
  }
}

// ============================================================================
// SERIAL MESSAGE QUEUE
// ============================================================================

void queueMessage(const String& msg) {
  if (queueCount < MESSAGE_QUEUE_SIZE) {
    messageQueue[queueTail] = msg;
    queueTail = (queueTail + 1) % MESSAGE_QUEUE_SIZE;
    queueCount++;
  } else {
    Serial.println("WARNING: Message queue full, dropping message");
  }
}

void processMessageQueue() {
  while (queueCount > 0) {
    sendToAllInterfaces(messageQueue[queueHead]);
    queueHead = (queueHead + 1) % MESSAGE_QUEUE_SIZE;
    queueCount--;
  }
}

// ============================================================================
// LED CONTROL
// ============================================================================

void sendLEDCommand(uint8_t nodeId, LEDState state) {
  if (nodeId < 1 || nodeId > NUM_BUZZERS) return;

  BuzzerMessage msg;
  msg.node_id = nodeId;
  msg.msg_type = MSG_LED_COMMAND;
  msg.value = state;
  msg.timestamp = millis();

  esp_now_send(buzzerMACs[nodeId - 1], (uint8_t*)&msg, sizeof(msg));
}

void updateAllLEDs() {
  for (uint8_t i = 1; i <= NUM_BUZZERS; i++) {
    if (currentState == STATE_READY) {
      // All LEDs on in ready state
      sendLEDCommand(i, LED_ON);
    } else if (currentState == STATE_LOCKED) {
      // Selected buzzer blinks, others off
      if (i == selectedBuzzer) {
        sendLEDCommand(i, LED_BLINK);
      } else {
        sendLEDCommand(i, LED_OFF);
      }
    } else if (currentState == STATE_PARTIAL_LOCKOUT) {
      // Selected buzzer blinks, locked buzzers off, active buzzers on
      if (i == selectedBuzzer) {
        sendLEDCommand(i, LED_BLINK);
      } else if (lockedBuzzers & (1 << (i - 1))) {
        sendLEDCommand(i, LED_OFF);
      } else {
        sendLEDCommand(i, LED_ON);
      }
    }
  }
}

// ============================================================================
// CONNECTION MONITORING & HEARTBEAT
// ============================================================================

void broadcastHeartbeat() {
  BuzzerMessage msg;
  msg.node_id = 0; // 0 = broadcast from controller
  msg.msg_type = MSG_HEARTBEAT;
  msg.value = 0;
  msg.timestamp = millis();

  // Send to each buzzer individually (more reliable than broadcast)
  for (int i = 0; i < NUM_BUZZERS; i++) {
    esp_now_send(buzzerMACs[i], (uint8_t*)&msg, sizeof(msg));
  }
}

void updateNodeConnection(uint8_t nodeId) {
  if (nodeId < 1 || nodeId > NUM_BUZZERS) return;
  
  unsigned long now = millis();
  bool wasConnected = nodeConnected[nodeId - 1];
  nodeLastSeen[nodeId - 1] = now;
  nodeConnected[nodeId - 1] = true;

  if (!wasConnected) {
    // Node reconnected
    String msg = "RECONNECT:" + String(nodeId);
    sendToAllInterfaces(msg);
  }
}

void checkNodeTimeouts() {
  unsigned long now = millis();
  
  for (uint8_t i = 0; i < NUM_BUZZERS; i++) {
    if (nodeConnected[i]) {
      if (now - nodeLastSeen[i] > CONNECTION_TIMEOUT_MS) {
        // Node timed out
        nodeConnected[i] = false;
        String msg = "DISCONNECT:" + String(i + 1);
        sendToAllInterfaces(msg);
      }
    }
  }
}

void sendStateSync(uint8_t nodeId) {
  if (nodeId < 1 || nodeId > NUM_BUZZERS) return;

  BuzzerMessage msg;
  msg.node_id = nodeId;
  msg.msg_type = MSG_STATE_SYNC;
  msg.timestamp = millis();
  
  // Pack game state into value field:
  // Bits 0-3: locked buzzers bitmask
  // Bits 4-6: selected buzzer (0-4)
  // Bit 7: game state mode (0=LOCKED, 1=PARTIAL_LOCKOUT)
  msg.value = lockedBuzzers | (selectedBuzzer << 4);
  if (currentState == STATE_PARTIAL_LOCKOUT) {
    msg.value |= 0x80; // Set bit 7 for PARTIAL_LOCKOUT
  }

  esp_now_send(buzzerMACs[nodeId - 1], (uint8_t*)&msg, sizeof(msg));
  
  Serial.print("STATE_SYNC:");
  Serial.print(nodeId);
  Serial.print(" (state=");
  Serial.print(currentState);
  Serial.print(", selected=");
  Serial.print(selectedBuzzer);
  Serial.print(", locked=0x");
  Serial.print(lockedBuzzers, HEX);
  Serial.println(")");
}

// ============================================================================
// GAME STATE HANDLERS
// ============================================================================

void handleBuzzerPress(uint8_t nodeId, uint32_t timestamp) {
  if (nodeId < 1 || nodeId > NUM_BUZZERS) return;

  // Check if this buzzer is locked out
  if (lockedBuzzers & (1 << (nodeId - 1))) {
    Serial.print("Buzzer ");
    Serial.print(nodeId);
    Serial.println(" is locked out, ignoring press");
    return;
  }

  if (currentState == STATE_READY || currentState == STATE_PARTIAL_LOCKOUT) {
    // Accept the press
    selectedBuzzer = nodeId;
    currentState = STATE_LOCKED;
    lastPressTime = timestamp;

    Serial.print("Buzzer ");
    Serial.print(nodeId);
    Serial.println(" pressed and locked in");

    // Send to PC/BLE clients
    queueMessage("BUZZ " + String(nodeId));

    // Update LEDs: selected blinks, others off
    updateAllLEDs();
  } else if (currentState == STATE_LOCKED) {
    // Already locked, ignore subsequent presses
    Serial.print("System locked, ignoring press from buzzer ");
    Serial.println(nodeId);
  }
}

void handleCorrectAnswer() {
  if (selectedBuzzer == 0) {
    Serial.println("No buzzer selected, ignoring CORRECT command");
    return;
  }

  Serial.println("CORRECT answer - resetting to READY");
  
  // Reset to ready state
  currentState = STATE_READY;
  selectedBuzzer = 0;
  lockedBuzzers = 0;

  // Send to PC/BLE clients
  queueMessage("CORRECT");

  // All LEDs on
  updateAllLEDs();
}

void handleWrongAnswer() {
  if (selectedBuzzer == 0) {
    Serial.println("No buzzer selected, ignoring WRONG command");
    return;
  }

  Serial.print("WRONG answer from buzzer ");
  Serial.print(selectedBuzzer);
  Serial.println(" - entering PARTIAL_LOCKOUT");

  // Lock out the wrong buzzer
  lockedBuzzers |= (1 << (selectedBuzzer - 1));
  
  // Check if all buzzers are now locked
  if (lockedBuzzers == 0x0F) { // All 4 buzzers locked (bits 0-3 set)
    Serial.println("All buzzers locked out, resetting to READY");
    currentState = STATE_READY;
    selectedBuzzer = 0;
    lockedBuzzers = 0;
  } else {
    // Enter partial lockout state
    currentState = STATE_PARTIAL_LOCKOUT;
    selectedBuzzer = 0; // Clear selection so another buzzer can try
  }

  // Send to PC/BLE clients
  queueMessage("WRONG");

  // Update LEDs
  updateAllLEDs();
}

void handleFullReset() {
  Serial.println("FULL RESET - clearing all state");

  // Reset everything
  currentState = STATE_READY;
  selectedBuzzer = 0;
  lockedBuzzers = 0;

  // Send to PC/BLE clients
  queueMessage("RESET");

  // All LEDs on
  updateAllLEDs();
}

// ============================================================================
// ESP-NOW CALLBACKS
// ============================================================================

void onDataReceive(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(BuzzerMessage)) {
    Serial.println("ERROR: Received message with wrong size");
    return;
  }

  BuzzerMessage msg;
  memcpy(&msg, data, sizeof(msg));

  // Update connection tracking for any message from a node
  updateNodeConnection(msg.node_id);

  // Process message based on type
  if (msg.msg_type == MSG_BUTTON_PRESS) {
    handleBuzzerPress(msg.node_id, msg.timestamp);
  } else if (msg.msg_type == MSG_STATE_REQUEST) {
    // Node is requesting current game state (reconnection)
    Serial.print("State request from node ");
    Serial.println(msg.node_id);
    sendStateSync(msg.node_id);
  }
}

void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  // LED commands are fire-and-forget, no need to handle send status
  // Could add retry logic here if needed
}

// ============================================================================
// CONTROL BUTTON HANDLING
// ============================================================================

void handleControlButtons() {
  // Handle CORRECT button
  int correctReading = digitalRead(CTRL_BUTTON_CORRECT);
  if (correctReading != lastCorrectState) {
    lastCorrectDebounce = millis();
  }
  if ((millis() - lastCorrectDebounce) > DEBOUNCE_DELAY_MS) {
    if (correctReading == LOW) { // Button pressed (pullup)
      handleCorrectAnswer();
      while (digitalRead(CTRL_BUTTON_CORRECT) == LOW) delay(10); // Wait for release
    }
  }
  lastCorrectState = correctReading;

  // Handle WRONG button
  int wrongReading = digitalRead(CTRL_BUTTON_WRONG);
  if (wrongReading != lastWrongState) {
    lastWrongDebounce = millis();
  }
  if ((millis() - lastWrongDebounce) > DEBOUNCE_DELAY_MS) {
    if (wrongReading == LOW) {
      handleWrongAnswer();
      while (digitalRead(CTRL_BUTTON_WRONG) == LOW) delay(10);
    }
  }
  lastWrongState = wrongReading;

  // Handle RESET button
  int resetReading = digitalRead(CTRL_BUTTON_RESET);
  if (resetReading != lastResetState) {
    lastResetDebounce = millis();
  }
  if ((millis() - lastResetDebounce) > DEBOUNCE_DELAY_MS) {
    if (resetReading == LOW) {
      handleFullReset();
      while (digitalRead(CTRL_BUTTON_RESET) == LOW) delay(10);
    }
  }
  lastResetState = resetReading;
}

// ============================================================================
// SERIAL COMMAND INPUT
// ============================================================================

void handleSerialInput() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    // Handle newline characters (command terminator)
    if (c == '\n' || c == '\r') {
      // Only process if buffer has content
      if (serialInputIndex > 0) {
        // Null-terminate the command string
        serialInputBuffer[serialInputIndex] = '\0';
        
        // Parse and execute the command
        String command(serialInputBuffer);
        command.trim(); // Remove any whitespace
        
        if (command == "CORRECT") {
          Serial.println("CMD_ACK:CORRECT");
          handleCorrectAnswer();
        } else if (command == "WRONG") {
          Serial.println("CMD_ACK:WRONG");
          handleWrongAnswer();
        } else if (command == "RESET") {
          Serial.println("CMD_ACK:RESET");
          handleFullReset();
        } else if (command.length() > 0) {
          // Unknown command
          Serial.print("CMD_ERR:UNKNOWN:");
          Serial.println(command);
        }
        
        // Reset buffer for next command
        serialInputIndex = 0;
      }
    } else {
      // Add character to buffer if there's space
      if (serialInputIndex < SERIAL_INPUT_BUFFER_SIZE - 1) {
        serialInputBuffer[serialInputIndex++] = c;
      } else {
        // Buffer overflow - discard and report error
        Serial.println("CMD_ERR:BUFFER_OVERFLOW");
        serialInputIndex = 0;
      }
    }
  }
}

// ============================================================================
// BLE INITIALIZATION
// ============================================================================

void initBLE() {
  Serial.println("========================================");
  Serial.println("INITIALIZING BLE");
  Serial.println("========================================");
  
  // Get MAC address to create unique device name
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  
  // Create device name with last 4 MAC digits
  char deviceName[32];
  snprintf(deviceName, sizeof(deviceName), "%s-%02X%02X", 
           BLE_DEVICE_NAME, mac[4], mac[5]);
  bleDeviceName = String(deviceName);
  
  Serial.print("BLE Device Name: ");
  Serial.println(bleDeviceName);
  
  // Initialize BLE
  BLEDevice::init(bleDeviceName.c_str());
  
  // Create BLE Server
  pBLEServer = BLEDevice::createServer();
  pBLEServer->setCallbacks(new ServerCallbacks());
  
  // Create Nordic UART Service
  BLEService *pService = pBLEServer->createService(BLE_SERVICE_UUID);
  
  // Create TX Characteristic (ESP32 -> Client, notifications)
  pTxCharacteristic = pService->createCharacteristic(
    BLE_TX_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
  );
  pTxCharacteristic->addDescriptor(new BLE2902()); // Enable notifications
  
  // Create RX Characteristic (Client -> ESP32, write)
  pRxCharacteristic = pService->createCharacteristic(
    BLE_RX_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  pRxCharacteristic->setCallbacks(new RxCallbacks());
  
  // Start service
  pService->start();
  Serial.println("✓ BLE service started");
  
  // Configure advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // Connection interval: 7.5ms
  pAdvertising->setMaxPreferred(0x12);  // Connection interval: 22.5ms
  
  // Start advertising
  BLEDevice::startAdvertising();
  Serial.println("✓ BLE advertising started");
  Serial.println("Clients can now connect via BLE");
  Serial.println("========================================");
}

// ============================================================================
// SETUP AND MAIN LOOP
// ============================================================================

void setup() {
  // Initialize serial for PC communication
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);

  Serial.println("========================================");
  Serial.println("MAIN CONTROLLER");
  Serial.println("========================================");

  // Configure control button pins
  pinMode(CTRL_BUTTON_CORRECT, INPUT_PULLUP);
  pinMode(CTRL_BUTTON_WRONG, INPUT_PULLUP);
  pinMode(CTRL_BUTTON_RESET, INPUT_PULLUP);

  // Set custom MAC address (AA:BB:CC:DD:EE:00 for main controller)
  WiFi.mode(WIFI_STA);
  uint8_t customMAC[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00};
  esp_err_t macResult = esp_wifi_set_mac(WIFI_IF_STA, customMAC);

  Serial.print("Custom MAC address: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", customMAC[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  if (macResult == ESP_OK) {
    Serial.println("✓ Custom MAC address set successfully");
  } else {
    Serial.println("✗ ERROR: Failed to set custom MAC address");
  }

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ ERROR: ESP-NOW initialization failed");
    return;
  }
  Serial.println("✓ ESP-NOW initialized");

  // Register callbacks
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceive);

  // Add all buzzer nodes as peers
  for (int i = 0; i < NUM_BUZZERS; i++) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, buzzerMACs[i], 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.print("✗ ERROR: Failed to add buzzer ");
      Serial.print(i + 1);
      Serial.println(" as peer");
    } else {
      Serial.print("✓ Buzzer ");
      Serial.print(i + 1);
      Serial.println(" added as peer");
    }
  }

  // Initialize BLE
  initBLE();

  Serial.println("========================================");
  Serial.println("Main controller ready!");
  Serial.println("- ESP-NOW for buzzer nodes");
  Serial.println("- USB Serial for commands");
  Serial.println("- BLE for wireless clients");
  Serial.println("Initializing all LEDs to ON (READY state)");
  Serial.println("========================================");

  // Initialize all LEDs to ON
  delay(500); // Give buzzer nodes time to initialize
  updateAllLEDs();
}

void loop() {
  // Broadcast heartbeat periodically
  unsigned long now = millis();
  if (now - lastHeartbeatTime >= HEARTBEAT_INTERVAL_MS) {
    broadcastHeartbeat();
    lastHeartbeatTime = now;
  }

  // Check for node timeouts
  checkNodeTimeouts();

  handleControlButtons();
  handleSerialInput();
  processMessageQueue();
  delay(1); // Small delay to prevent watchdog issues
}
