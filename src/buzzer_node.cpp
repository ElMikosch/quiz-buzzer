#include "config.h"
#include "protocol.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <esp_pm.h>

// Ensure NODE_ID is defined at compile time
#ifndef NODE_ID
#error "NODE_ID must be defined (1-4) via build flags"
#endif

#if NODE_ID < 1 || NODE_ID > 4
#error "NODE_ID must be between 1 and 4"
#endif

// ============================================================================
// GLOBAL STATE
// ============================================================================

// Power mode management for battery optimization
enum PowerMode {
  ACTIVE,      // Ready state: 50ms sleep, GPIO wake enabled, fast button response
  LOW_POWER,   // Locked state: 100ms sleep, GPIO wake disabled, button inactive
  MINIMAL      // Disconnected state: 200ms sleep, GPIO wake disabled, breathing LED
};

PowerMode currentPowerMode = MINIMAL; // Start in minimal power mode (disconnected)

// LED state management
LEDState currentLEDState = LED_OFF;
LEDState savedLEDState = LED_OFF; // Save state before disconnection
unsigned long lastBlinkTime = 0;
bool blinkState = false;

// LED fade variables for breathing effect
// Breathing fade is used during disconnected state for smooth visual feedback
uint8_t fadeBrightness = 0;        // Current brightness (0-255)
int8_t fadeDirection = 1;          // 1 for fade-in, -1 for fade-out
unsigned long lastFadeTime = 0;    // Last time fade was updated

// Two-stage blink variables for pressed state
// Fast blink (5Hz) for 3 seconds grabs attention, then slow blink (2Hz) continues
unsigned long fastBlinkStartTime = 0;  // When fast blink phase started
bool isInFastBlinkPhase = false;       // True if in 5Hz fast blink phase

// Button state management
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

// Connection monitoring
unsigned long lastHeartbeatTime = 0;
bool isConnected = false;

// Main controller MAC address (will be set to AA:BB:CC:DD:EE:00)
uint8_t mainControllerMAC[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00};

// ============================================================================
// POWER MANAGEMENT
// ============================================================================

// Set power mode and configure sleep parameters accordingly
void setPowerMode(PowerMode mode) {
  if (currentPowerMode == mode) {
    return; // No change needed
  }
  
  PowerMode oldMode = currentPowerMode;
  currentPowerMode = mode;
  
  // Log power mode transition
  Serial.print("Power mode transition: ");
  switch (oldMode) {
    case ACTIVE: Serial.print("ACTIVE"); break;
    case LOW_POWER: Serial.print("LOW_POWER"); break;
    case MINIMAL: Serial.print("MINIMAL"); break;
  }
  Serial.print(" -> ");
  switch (mode) {
    case ACTIVE: Serial.println("ACTIVE"); break;
    case LOW_POWER: Serial.println("LOW_POWER"); break;
    case MINIMAL: Serial.println("MINIMAL"); break;
  }
}

// Enter light sleep with appropriate wake sources
void enterLightSleep(uint64_t duration_us) {
  // Configure timer wake as backup
  esp_sleep_enable_timer_wakeup(duration_us);
  
  // Configure GPIO wake only in ACTIVE mode (button responsive)
  if (currentPowerMode == ACTIVE) {
    // Enable GPIO wake on button press (LOW level due to pullup)
    gpio_wakeup_enable((gpio_num_t)BUZZER_BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
  } else {
    // Disable GPIO wake in low power modes (button inactive)
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
  }
  
  // Enter light sleep (wakes on GPIO interrupt or timer)
  esp_light_sleep_start();
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

  // Handle heartbeat messages from controller
  if (msg.msg_type == MSG_HEARTBEAT) {
    unsigned long now = millis();
    bool wasConnected = isConnected;
    lastHeartbeatTime = now;
    
    if (!wasConnected) {
      // We just reconnected
      Serial.println("Connected to controller");
      isConnected = true;
      
      // Request current game state
      BuzzerMessage stateReq;
      stateReq.node_id = NODE_ID;
      stateReq.msg_type = MSG_STATE_REQUEST;
      stateReq.value = 0;
      stateReq.timestamp = now;
      
      Serial.println("Requesting state sync...");
      esp_now_send(mainControllerMAC, (uint8_t *)&stateReq, sizeof(stateReq));
    }
    return;
  }

  // Handle state sync messages
  if (msg.msg_type == MSG_STATE_SYNC && msg.node_id == NODE_ID) {
    Serial.println("=== STATE SYNC RECEIVED ===");
    
    // Unpack game state from value field
    uint8_t lockedBuzzers = msg.value & 0x0F;     // Bits 0-3
    uint8_t selectedBuzzer = (msg.value >> 4) & 0x07; // Bits 4-6
    bool isPartialLockout = (msg.value & 0x80) != 0;  // Bit 7
    
    Serial.print("  Locked buzzers: 0x");
    Serial.print(lockedBuzzers, HEX);
    Serial.print(" | Selected buzzer: ");
    Serial.print(selectedBuzzer);
    Serial.print(" | Mode: ");
    Serial.println(isPartialLockout ? "PARTIAL_LOCKOUT" : "LOCKED");
    
    // Determine correct LED state based on game state
    bool isLocked = lockedBuzzers & (1 << (NODE_ID - 1));
    bool isSelected = (selectedBuzzer == NODE_ID);
    
    Serial.print("  This node: locked=");
    Serial.print(isLocked ? "YES" : "NO");
    Serial.print(" | selected=");
    Serial.println(isSelected ? "YES" : "NO");
    
    if (isSelected) {
      currentLEDState = LED_BLINK;
      lastBlinkTime = millis(); // Reset blink timer to start immediately
      // Start two-stage blink: fast blink for 3 seconds, then slow
      isInFastBlinkPhase = true;
      fastBlinkStartTime = millis();
      Serial.println("  -> LED state: BLINK (selected)");
      setPowerMode(LOW_POWER); // Button locked, use low power mode
    } else if (selectedBuzzer == 0) {
      // STATE_READY: no buzzer selected, all LEDs ON
      currentLEDState = LED_ON;
      Serial.println("  -> LED state: ON (ready state)");
      setPowerMode(ACTIVE); // Button active, need fast response
    } else if (isPartialLockout) {
      // In PARTIAL_LOCKOUT: only explicitly locked buzzers turn OFF
      if (isLocked) {
        currentLEDState = LED_OFF;
        Serial.println("  -> LED state: OFF (locked in PARTIAL_LOCKOUT)");
        setPowerMode(LOW_POWER); // Button locked, use low power mode
      } else {
        currentLEDState = LED_ON;
        Serial.println("  -> LED state: ON (not locked in PARTIAL_LOCKOUT)");
        setPowerMode(ACTIVE); // Button active, need fast response
      }
    } else {
      // In LOCKED state: all non-selected buzzers turn OFF
      currentLEDState = LED_OFF;
      Serial.println("  -> LED state: OFF (not selected in LOCKED)");
      setPowerMode(LOW_POWER); // Button locked, use low power mode
    }
    
    savedLEDState = currentLEDState;
    Serial.println("=== STATE SYNC COMPLETE ===");
    return;
  }

  // Handle LED commands for this node
  if (msg.node_id == NODE_ID && msg.msg_type == MSG_LED_COMMAND) {
    currentLEDState = (LEDState)msg.value;
    savedLEDState = currentLEDState; // Save in case of disconnection
    Serial.print("LED command received: ");
    Serial.println(msg.value);

    // Handle state-specific initialization and power mode transitions
    if (currentLEDState == LED_ON) {
      ledcWrite(LED_PWM_CHANNEL, 255);  // Full brightness
      blinkState = true;
      setPowerMode(ACTIVE); // Button active, need fast response
    } else if (currentLEDState == LED_OFF) {
      ledcWrite(LED_PWM_CHANNEL, 0);  // Off
      blinkState = false;
      setPowerMode(LOW_POWER); // Button locked, use low power mode
    } else if (currentLEDState == LED_BLINK) {
      // Start two-stage blink pattern
      isInFastBlinkPhase = true;
      fastBlinkStartTime = millis();
      lastBlinkTime = millis();
      setPowerMode(LOW_POWER); // Button locked (selected buzzer), use low power mode
    } else if (currentLEDState == LED_FADE) {
      // Initialize fade state
      fadeBrightness = 0;
      fadeDirection = 1;
      lastFadeTime = millis();
      setPowerMode(MINIMAL); // Disconnected, use minimal power mode
    }
  }
}

void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Button press sent successfully");
  } else {
    Serial.println("ERROR: Button press send failed");
  }
}

// ============================================================================
// BUTTON HANDLING
// ============================================================================

void handleButton() {
  int reading = digitalRead(BUZZER_BUTTON_PIN);

  // Check if button state changed
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // Only register press after debounce delay
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    // Button pressed (LOW due to pullup)
    if (reading == LOW) {
      // Send button press message
      BuzzerMessage msg;
      msg.node_id = NODE_ID;
      msg.msg_type = MSG_BUTTON_PRESS;
      msg.value = 1;
      msg.timestamp = millis();

      Serial.print("Button pressed! Sending message from node ");
      Serial.println(NODE_ID);

      // Send with retries
      for (int i = 0; i < MAX_RETRIES; i++) {
        esp_err_t result =
            esp_now_send(mainControllerMAC, (uint8_t *)&msg, sizeof(msg));
        if (result == ESP_OK) {
          break;
        }
        delay(RETRY_INTERVAL_MS);
      }

      // Wait for button release to avoid multiple presses
      while (digitalRead(BUZZER_BUTTON_PIN) == LOW) {
        delay(10);
      }
    }
  }

  lastButtonState = reading;
}

// ============================================================================
// CONNECTION MONITORING
// ============================================================================

void checkConnection() {
  unsigned long now = millis();
  bool wasConnected = isConnected;
  
  // Check if we've timed out
  if (isConnected && (now - lastHeartbeatTime > CONNECTION_TIMEOUT_MS)) {
    isConnected = false;
    Serial.println("Disconnected from controller (timeout)");
    
    // Save current LED state before entering disconnected mode
    savedLEDState = currentLEDState;
    
    // Enter breathing fade mode to indicate disconnection
    currentLEDState = LED_FADE;
    fadeBrightness = 0;
    fadeDirection = 1;
    lastFadeTime = now;
    
    // Enter minimal power mode for maximum battery savings
    setPowerMode(MINIMAL);
  }
}

// ============================================================================
// LED HANDLING
// ============================================================================

// Smooth breathing fade effect for disconnected state
// Uses PWM to gradually fade brightness in and out
void handleLEDFade() {
  unsigned long now = millis();
  
  // Update fade brightness at configured interval
  if (now - lastFadeTime >= FADE_INTERVAL_MS) {
    lastFadeTime = now;
    
    // Update brightness based on fade direction
    fadeBrightness += (fadeDirection * FADE_STEP);
    
    // Reverse direction at brightness limits
    if (fadeBrightness <= 0) {
      fadeBrightness = 0;
      fadeDirection = 1;  // Start fading in
    } else if (fadeBrightness >= 255) {
      fadeBrightness = 255;
      fadeDirection = -1;  // Start fading out
    }
    
    // Apply brightness using PWM
    ledcWrite(LED_PWM_CHANNEL, fadeBrightness);
  }
}

void handleLED() {
  unsigned long now = millis();
  
  switch (currentLEDState) {
  case LED_ON:
    // Solid on at full brightness using PWM
    ledcWrite(LED_PWM_CHANNEL, 255);
    break;

  case LED_OFF:
    // Off (zero brightness) using PWM
    ledcWrite(LED_PWM_CHANNEL, 0);
    break;

  case LED_FADE:
    // Breathing fade effect (for disconnected state)
    handleLEDFade();
    break;

  case LED_BLINK:
    // Two-stage blink pattern: 5Hz fast for 3 seconds, then 2Hz slow
    unsigned long blinkInterval;
    
    // Determine if we're in fast blink phase or slow blink phase
    if (isInFastBlinkPhase) {
      unsigned long elapsedTime = now - fastBlinkStartTime;
      
      if (elapsedTime >= FAST_BLINK_DURATION_MS) {
        // Transition to slow blink phase
        isInFastBlinkPhase = false;
        blinkInterval = BLINK_INTERVAL_MS;  // 2Hz (500ms)
      } else {
        // Stay in fast blink phase
        blinkInterval = FAST_BLINK_INTERVAL_MS;  // 5Hz (100ms)
      }
    } else {
      // Normal slow blink
      blinkInterval = BLINK_INTERVAL_MS;  // 2Hz (500ms)
    }
    
    // Execute non-blocking blink using PWM
    if (now - lastBlinkTime >= blinkInterval) {
      blinkState = !blinkState;
      ledcWrite(LED_PWM_CHANNEL, blinkState ? 255 : 0);
      lastBlinkTime = now;
    }
    break;
  }
}

// ============================================================================
// SETUP AND MAIN LOOP
// ============================================================================

void setup() {
  // Initialize serial for debugging
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);

  Serial.println("========================================");
  Serial.print("BUZZER NODE ");
  Serial.println(NODE_ID);
  Serial.println("========================================");

  // Set CPU frequency for power optimization
  setCpuFrequencyMhz(CPU_FREQUENCY_MHZ);
  uint32_t actualFreq = getCpuFrequencyMhz();
  Serial.print("✓ CPU frequency set to ");
  Serial.print(actualFreq);
  Serial.println(" MHz");

  // Configure GPIO pins
  pinMode(BUZZER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_LED_PIN, OUTPUT);
  digitalWrite(BUZZER_LED_PIN, LOW);

  // Configure GPIO wake capability for light sleep
  gpio_wakeup_enable((gpio_num_t)BUZZER_BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
  Serial.println("✓ GPIO wake configured for button pin");

  // Initialize PWM/LEDC for smooth LED control
  ledcSetup(LED_PWM_CHANNEL, LED_PWM_FREQUENCY, LED_PWM_RESOLUTION);
  ledcAttachPin(BUZZER_LED_PIN, LED_PWM_CHANNEL);
  ledcWrite(LED_PWM_CHANNEL, 0); // Start with LED off
  Serial.println("✓ PWM/LEDC initialized for LED control");

  // Set custom MAC address
  WiFi.mode(WIFI_STA);
  uint8_t customMAC[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, NODE_ID};
  esp_err_t macResult = esp_wifi_set_mac(WIFI_IF_STA, customMAC);

  Serial.print("Custom MAC address: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", customMAC[i]);
    if (i < 5)
      Serial.print(":");
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

  // Enable WiFi modem sleep for power optimization
  esp_err_t psResult = esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
  if (psResult == ESP_OK) {
    Serial.println("✓ WiFi modem sleep enabled");
  } else {
    Serial.println("✗ WARNING: Failed to enable WiFi modem sleep");
  }

  // Register callbacks
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceive);

  // Add main controller as peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mainControllerMAC, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("✗ ERROR: Failed to add main controller as peer");
    return;
  }
  Serial.println("✓ Main controller added as peer");

  // Initial LED state: breathing fade (disconnected until first heartbeat)
  currentLEDState = LED_FADE;
  savedLEDState = LED_OFF;
  fadeBrightness = 0;
  fadeDirection = 1;
  lastFadeTime = millis();

  // Initialize connection state (start as disconnected, will connect on first heartbeat)
  isConnected = false;
  lastHeartbeatTime = millis(); // Initialize to current time

  // Start in minimal power mode (disconnected)
  currentPowerMode = MINIMAL;

  Serial.println("========================================");
  Serial.println("Buzzer node ready!");
  Serial.println("Waiting for controller heartbeat...");
  Serial.println("========================================");
}

void loop() {
  checkConnection();
  handleButton();
  handleLED();
  
  // Select sleep duration based on current power mode
  uint64_t sleepDuration;
  switch (currentPowerMode) {
    case ACTIVE:
      sleepDuration = LIGHT_SLEEP_DURATION_READY_US; // 50ms - fast button response
      break;
    case LOW_POWER:
      sleepDuration = LIGHT_SLEEP_DURATION_LOCKED_US; // 100ms - button inactive
      break;
    case MINIMAL:
      sleepDuration = LIGHT_SLEEP_DURATION_DISCONNECTED_US; // 200ms - disconnected
      break;
    default:
      sleepDuration = LIGHT_SLEEP_DURATION_READY_US;
      break;
  }
  
  // Enter light sleep with appropriate wake sources
  enterLightSleep(sleepDuration);
}
