#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <Arduino.h>

// Message types
enum MessageType : uint8_t {
  MSG_BUTTON_PRESS = 1,
  MSG_LED_COMMAND = 2,
  MSG_ACK = 3,
  MSG_HEARTBEAT = 4,
  MSG_STATE_REQUEST = 5,
  MSG_STATE_SYNC = 6
};

// LED states
enum LEDState : uint8_t {
  LED_OFF = 0,    // LED off (locked out or other buzzer selected)
  LED_ON = 1,     // LED solid on (ready state, full brightness)
  LED_BLINK = 2,  // Two-stage blink: 5Hz fast for 3s, then 2Hz slow (selected buzzer)
  LED_FADE = 3    // Breathing fade effect for disconnected state (smooth in/out)
};

// ESP-NOW message structure
struct BuzzerMessage {
  uint8_t node_id;      // 1-4 for buzzer nodes
  uint8_t msg_type;     // MessageType enum
  uint8_t value;        // LED state or press count
                        // For MSG_STATE_SYNC: bits 0-3 = locked buzzers bitmask
                        //                     bits 4-6 = selected buzzer (0-4)
                        //                     bit 7    = game state mode (0=LOCKED, 1=PARTIAL_LOCKOUT)
  uint32_t timestamp;   // millis() for deduplication
};

// Custom MAC address base
const uint8_t MAC_BASE[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00};

#endif // PROTOCOL_H
