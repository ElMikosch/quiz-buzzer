#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// PIN ASSIGNMENTS
// ============================================================================

// Buzzer Node Pins
#define BUZZER_BUTTON_PIN 12 // Button input with internal pullup
#define BUZZER_LED_PIN 13    // LED output
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

// PWM/LEDC Configuration for smooth LED control
// ESP32 LEDC peripheral provides hardware PWM for brightness control
#define LED_PWM_CHANNEL 0       // LEDC channel (0-15 available)
#define LED_PWM_FREQUENCY 5000  // PWM frequency in Hz (5kHz recommended to avoid flicker)
#define LED_PWM_RESOLUTION 8    // 8-bit resolution gives 0-255 brightness levels

// LED Fade Configuration for breathing effect
// Breathing effect creates smooth fade in/out during disconnected state
#define FADE_STEP 5             // Brightness increment per fade step (smaller = smoother)
#define FADE_INTERVAL_MS 20     // Time between fade steps (achieves ~2-3s per cycle)

// Two-stage Blink Configuration for pressed buzzer feedback
// Fast initial blink grabs attention, then transitions to slower sustained blink
#define FAST_BLINK_DURATION_MS 3000  // Duration of fast blink phase (3 seconds)
#define FAST_BLINK_INTERVAL_MS 100   // Fast blink interval: 5Hz (100ms on/off)

// ============================================================================
// COMMUNICATION CONSTANTS
// ============================================================================

#define SERIAL_BAUD_RATE 115200      // USB serial baud rate
#define MESSAGE_QUEUE_SIZE 10        // Maximum queued serial messages
#define ESPNOW_CHANNEL 1             // ESP-NOW WiFi channel (1-13)
#define SERIAL_INPUT_BUFFER_SIZE 256 // Buffer size for serial command input

// ============================================================================
// GAME CONFIGURATION
// ============================================================================

#define NUM_BUZZERS 4 // Total number of buzzer nodes

#endif // CONFIG_H
