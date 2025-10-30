#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// PIN ASSIGNMENTS
// ============================================================================

// Buzzer Node Pins
#define BUZZER_BUTTON_PIN 25 // Button input with internal pullup
#define BUZZER_LED_PIN 27    // LED output
#define BUZZER_SPEAKER_PIN                                                     \
  14 // Reserved for future speaker (not implemented in v1)

// Main Controller Pins
#define CTRL_BUTTON_CORRECT 25 // Correct answer button (with internal pullup)
#define CTRL_BUTTON_WRONG 26   // Wrong answer button (with internal pullup)
#define CTRL_BUTTON_RESET 27   // Full reset button (with internal pullup)

// ============================================================================
// TIMING CONSTANTS
// ============================================================================

#define BLINK_INTERVAL_MS 500 // LED blink rate: 2Hz (500ms on, 500ms off)
#define DEBOUNCE_DELAY_MS 50  // Button debounce time
#define RETRY_INTERVAL_MS 10  // ESP-NOW retry interval
#define MAX_RETRIES 3         // Maximum message retransmission attempts

// Connection monitoring
#define HEARTBEAT_INTERVAL_MS 2000 // Send heartbeat every 2 seconds
#define CONNECTION_TIMEOUT_MS 5000 // Consider node disconnected after 5 seconds
#define DISCONNECT_BLINK_INTERVAL_MS                                           \
  100 // Fast blink when disconnected: 10Hz (100ms on, 100ms off)

// ============================================================================
// COMMUNICATION CONSTANTS
// ============================================================================

#define SERIAL_BAUD_RATE 115200 // USB serial baud rate
#define MESSAGE_QUEUE_SIZE 10   // Maximum queued serial messages
#define ESPNOW_CHANNEL 1        // ESP-NOW WiFi channel (1-13)

// ============================================================================
// GAME CONFIGURATION
// ============================================================================

#define NUM_BUZZERS 4 // Total number of buzzer nodes

#endif // CONFIG_H
