# Proposal: Add BLE Wireless Communication

## Change ID
`2025-11-23-add-ble-wireless-communication`

## Status
Draft

## Overview
Add Bluetooth Low Energy (BLE) GATT server to the main controller, enabling wireless bidirectional communication with PC and smartphone clients as an alternative to USB serial, while maintaining full backward compatibility with existing USB serial communication and ESP-NOW buzzer node communication.

## Problem Statement
The current system requires USB cable connection between the main controller and PC for game state updates and control commands. This creates several limitations:
- **Physical constraint**: Quiz master must be near the main controller (cable length ~2-3m)
- **Cable management**: USB cables can be trip hazards or get tangled
- **Mobile device support**: Smartphones cannot connect via USB serial easily
- **Flexibility**: Cannot easily switch control between multiple devices
- **Setup complexity**: Requires physical cable routing for each quiz setup

Users want the ability to monitor and control the quiz system wirelessly from PC or smartphone while maintaining the option to use USB serial for debugging and development.

## Proposed Solution
Implement a BLE GATT server on the main controller that:
1. Runs simultaneously with ESP-NOW and USB serial (uses separate Bluetooth radio)
2. Exposes game events via BLE characteristic notifications (buzzer presses, control actions)
3. Accepts control commands via BLE characteristic writes (CORRECT, WRONG, RESET)
4. Provides connection status and discovery via standard BLE advertising
5. Supports multiple client platforms: Windows, Linux, Android, iOS

### Why BLE Instead of WiFi?
- **No WiFi interference**: BLE uses separate radio, fully compatible with ESP-NOW operation
- **No network setup**: Works without WiFi network, access point, or router configuration
- **Better mobile support**: Native iOS and Android BLE APIs, no special permissions needed
- **Lower power**: BLE consumes less power than WiFi AP mode
- **Simpler architecture**: No IP addressing, routing, or network security configuration
- **ESP-NOW compatibility**: WiFi STA mode is used by ESP-NOW with custom MAC addressing, making traditional WiFi connections problematic

## Affected Components
- `main-controller`: Add BLE GATT server initialization, characteristic handlers, message bridging
- `pc-interface`: Extend specification to document BLE protocol alongside USB serial

## Benefits
1. **Wireless freedom**: Quiz master can control from anywhere within BLE range (~10-30m)
2. **Mobile support**: Native smartphone apps can connect and control the quiz
3. **Zero setup**: No network configuration, just pair and connect
4. **Full compatibility**: Existing USB serial functionality remains unchanged
5. **Parallel operation**: Both USB and BLE can be active simultaneously
6. **Debugging friendly**: USB serial remains available for development and troubleshooting
7. **No ESP-NOW conflict**: BLE radio is separate from WiFi radio used by ESP-NOW

## Risks and Mitigations
| Risk | Impact | Mitigation |
|------|--------|------------|
| BLE latency higher than USB serial | Medium | Acceptable for quiz scenarios (typical 50-100ms), test and document |
| Memory overhead from BLE stack | Medium | ESP32 has sufficient RAM (~320KB), monitor usage during development |
| Multiple simultaneous BLE clients | Low | Limit to 1 active BLE connection, document clearly |
| BLE vs USB message ordering | Low | Each transport independent, document behavior |
| Client development complexity | Medium | Provide reference implementations and clear protocol documentation |

## Alternatives Considered
1. **WiFi Access Point + WebSocket**: Rejected - Would work but requires clients to disconnect from internet WiFi
2. **WiFi Station + WebSocket**: Rejected - Conflicts with ESP-NOW's WIFI_STA usage and custom MAC addressing
3. **Bluetooth Classic SPP**: Rejected - Poor iOS support, not significantly better than BLE
4. **ESP-NOW for PC communication**: Rejected - Requires special WiFi hardware on PC, not suitable for smartphones

## Success Criteria
- [ ] BLE GATT server initializes without affecting ESP-NOW operation
- [ ] Game events (buzzer presses, control actions) transmitted via BLE notifications within 100ms
- [ ] Control commands (CORRECT, WRONG, RESET) received via BLE writes processed within 50ms
- [ ] USB serial continues to function normally when BLE is active
- [ ] Both USB and BLE can be used simultaneously without message loss
- [ ] Android client can successfully connect, receive events, and send commands
- [ ] Windows/Linux client can connect via native BLE or Web Bluetooth
- [ ] No increase in ESP-NOW message latency or packet loss
- [ ] Memory usage remains below 80% of available RAM

## Dependencies
- None - This change is self-contained within the main controller

## Timeline Estimate
- Design and spec: 1 day
- Implementation: 2-3 days
- Testing (hardware): 1-2 days
- Documentation and examples: 1 day
- **Total**: 5-7 days

## Related Changes
- None currently, but this enables future mobile app development

## Notes
- BLE GATT protocol provides standardized service/characteristic model
- Reference implementations should be provided for at least one platform (Android or Web Bluetooth)
- Consider adding battery level characteristic for future battery-powered operation
- BLE advertising name should be configurable or based on MAC address for multi-system scenarios
