# WCBClient

An Arduino library for ESP32 devices that lets them join a [Wireless Communication Board (WCB)](https://github.com/greghulette/Wireless_Communication_Board-WCB) ESP-NOW network as a first-class peer — sending commands, receiving commands, forwarding raw bytes to Pololu Maestro servo controllers, and participating in the ETM heartbeat system.

---

## What is a WCB?

WCB (Wireless Communication Board) is an ESP32-based wireless networking system designed for prop and animatronic control. Multiple WCB boards form a mesh over ESP-NOW, each with a unique ID. They exchange text commands (to trigger actions, run scripts, control servos) and raw serial data (to drive Pololu Maestro servo controllers wirelessly).

This library lets any ESP32 — a Kyber controller, a custom prop brain, a stage cue device — join that network without any hardware modification to the WCB boards.

---

## Requirements

- **Hardware:** Any ESP32 or ESP32-S3 board
- **Arduino IDE:** 2.x recommended
- **Board package:** `esp32 by Espressif Systems` v3.x
- **WCB firmware:** 6.0 or later on all WCB boards in the network

---

## Installation

1. Download or clone this repository
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library** and select the folder, or copy the folder into your Arduino `libraries/` directory
3. Restart Arduino IDE

---

## Quick Start

### Send and receive text commands

```cpp
#include <WCBClient.h>

WCBClient wcb(0x22, 0x33, "my_network_password", /*quantity=*/4, /*deviceID=*/20,
              [](uint8_t sender, const char* cmd) {
                  Serial.printf("From WCB%d: %s\n", sender, cmd);
              });

void setup() {
    Serial.begin(115200);
    wcb.begin();
}

void loop() {
    wcb.update();                    // must call every loop — drives heartbeats
    wcb.broadcast(";m11");          // send to all WCBs at once
    wcb.send(2, ":PP100");          // send to WCB2 only
    delay(5000);
}
```

### Forward Maestro servo commands wirelessly

```cpp
#include <WCBClient.h>
#include <WCBStream.h>
#include <PololuMaestro.h>

WCBClient wcb(0x22, 0x33, "my_network_password", 4, 20);

// broadcast — every WCB with a Maestro wired up will receive it
WCBStream maestroStream(broadcast);
MiniMaestro maestro(maestroStream, Maestro::noResetPin, /*deviceID=*/1);

void setup() {
    Serial.begin(115200);
    wcb.begin();
}

void loop() {
    wcb.update();                        // also flushes WCBStream automatically
    maestro.setTarget(0, 6000);          // 1500µs — centre position
    delay(1000);
    maestro.setTarget(0, 4000);          // 1000µs — full left
    delay(1000);
}
```

---

## API Reference

### WCBClient

| Method | Description |
|--------|-------------|
| `WCBClient(oct2, oct3, password, quantity, deviceID, [cmdCb], [statusCb])` | Constructor. `deviceID` must match your WCB system (1–19 within quantity, or 20 for the out-of-band special slot). |
| `begin()` | Initialize WiFi and ESP-NOW, set custom MAC, register peers. Call once from `setup()`. |
| `update()` | Call every `loop()`. Drives heartbeats, offline detection, and WCBStream flushing. |
| `send(wcbID, command)` | Unicast a text command to one WCB. |
| `broadcast(command)` | Broadcast a text command to all WCBs simultaneously. |
| `sendRaw(wcbID, port, data, len)` | Unicast raw bytes to a specific WCB's serial port (e.g., for a directly-wired Maestro). |
| `sendKyber(data, len)` | Broadcast raw bytes to all WCBs — any with Kyber_Remote will forward to their Maestro. |
| `monitorRaw(serial, target, port, [gap_ms])` | Watch a UART; flush buffered bytes via `sendRaw()` or `sendKyber()` (pass `broadcast` as target) when silent for `gap_ms`. |
| `monitorSerial(serial, target, [terminator])` | Watch a UART for newline-terminated text commands; forward each line via `send()` or `broadcast()`. |
| `isOnline(wcbID)` | Returns `true` if the specified WCB has sent a heartbeat recently. |
| `onCommand(callback)` | Register or replace the received-command callback. |
| `onStatusChange(callback)` | Register or replace the online/offline status callback. |
| `setChecksum(bool)` | Enable/disable CRC32 checksums. Must match `?ETM,CHKSM` setting on WCBs (default: on). |

### WCBStream

A `Stream` adapter that intercepts writes from the Pololu Maestro library and forwards them wirelessly instead of to a physical serial port.

```cpp
WCBStream stream(broadcast);        // broadcast to all WCBs with Kyber_Remote
WCBStream stream(wcbID, port);      // unicast to a specific WCB:port
WCBStream stream(wcbID, port, 5);   // unicast with 5ms inter-frame gap
```

Pass the `WCBStream` to the Pololu library in place of a serial port:

```cpp
MiniMaestro maestro(stream, Maestro::noResetPin, deviceNumber);
```

`wcb.update()` flushes all streams automatically — no extra code needed in `loop()`.

### Constants

| Constant | Value | Use |
|----------|-------|-----|
| `broadcast` | 0 | Pass as `target_wcb` to use Kyber broadcast path |
| `WCB_TARGET_BROADCAST` | 0 | Target ID for text command broadcasts |
| `WCB_TARGET_RAW_SERIAL` | 97 | Target ID for unicast raw serial forwarding |
| `WCB_TARGET_KYBER` | 98 | Target ID for Kyber broadcast raw forwarding |
| `WCB_SPECIAL_ID` | 20 | Out-of-band device slot (no WCB slot consumed) |

---

## Network Credentials

All devices on the same WCB network must share:

| Parameter | WCB Command | Description |
|-----------|-------------|-------------|
| `mac_oct2` / `mac_oct3` | `?WCBM` | MAC address octets 2 and 3 identifying the network |
| `password` | `?WCBP` | ESP-NOW network password |
| `wcb_quantity` | `?WCBQ` | Total number of WCB boards |

Query any WCB over serial or use the WCB Config Tool to find these values.

---

## Device ID Notes

- IDs **1 through `wcb_quantity`** are standard WCB slots. The WCB boards pre-register these MACs so packets arrive correctly. Your device ID must be ≤ `wcb_quantity`.
- ID **20** is the special out-of-band slot — it doesn't consume a WCB number and works even if `wcb_quantity` is less than 20, but requires `?SPECIAL,ON` to be set on the WCB boards.

---

## Examples

| Example | Description |
|---------|-------------|
| `BasicUsage` | Send and receive text commands |
| `MaestroUnicast` | Forward Maestro commands to a specific WCB:port |
| `MaestroBroadcast` | Broadcast Maestro commands to all WCBs with Maestros |
| `CombinedUsage` | Text commands and Maestro forwarding running simultaneously |

---

## License

MIT — see [LICENSE](LICENSE)
