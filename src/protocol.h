#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <Arduino.h>

// Message types
enum MessageType : uint8_t {
  MSG_BUTTON_PRESS = 1,
  MSG_LED_COMMAND = 2,
  MSG_ACK = 3
};

// LED states
enum LEDState : uint8_t {
  LED_OFF = 0,
  LED_ON = 1,
  LED_BLINK = 2
};

// ESP-NOW message structure
struct BuzzerMessage {
  uint8_t node_id;      // 1-4 for buzzer nodes
  uint8_t msg_type;     // MessageType enum
  uint8_t value;        // LED state or press count
  uint32_t timestamp;   // millis() for deduplication
};

// Custom MAC address base
const uint8_t MAC_BASE[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00};

#endif // PROTOCOL_H
