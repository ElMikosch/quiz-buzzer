## MODIFIED Requirements

### Requirement: Power-On Initialization
Each buzzer node SHALL initialize hardware and communication on power-up, including power management configuration for battery optimization.

#### Scenario: Successful initialization
- **WHEN** the node powers on
- **THEN** it SHALL set CPU frequency to 80MHz for power savings
- **AND** initialize GPIO pins (button input with pullup, LED output)
- **AND** configure button pin as GPIO wake source for light sleep
- **AND** reserve a GPIO pin for future speaker integration (not implemented)
- **AND** initialize PWM/LEDC for LED control
- **AND** set custom MAC address based on NODE_ID
- **AND** initialize WiFi and ESP-NOW with main controller MAC address
- **AND** enable WiFi modem sleep (WIFI_PS_MIN_MODEM)
- **AND** set LED to breathing fade (disconnected state until first heartbeat)
- **AND** print initialization status to Serial at 115200 baud
- **AND** include node ID, custom MAC address, and CPU frequency in output
