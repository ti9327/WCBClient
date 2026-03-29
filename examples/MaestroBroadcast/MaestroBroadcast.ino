/*
  MaestroBroadcast.ino — WCBClient + WCBStream Broadcast Example

  Demonstrates broadcasting Pololu Maestro servo commands to ALL WCBs on the
  network simultaneously using the  broadcast  constant. Any WCB that has a
  Maestro wired up and Kyber_Remote configured will execute the command.
  WCBs without a Maestro ignore it.

  Use this when:
    - You have Maestros on multiple WCBs and want to address all of them
    - You don't know (or don't care) which specific WCB has the Maestro
    - You want one command to ripple out to every servo controller at once

  For sending to one specific WCB:port, see MaestroUnicast.

  ── WCB setup required ───────────────────────────────────────────────────────
  On each WCB that has a Maestro physically wired to it:
    1. Configure the serial port:   ?MAESTRO,M1:W<n>S<port>:<baud>
    2. Enable Kyber remote mode:    ?KYBER,REMOTE,S<port>
  The WCB will then automatically forward any incoming Kyber broadcast to that
  serial port.

  ── Network credentials ───────────────────────────────────────────────────────
  Fill in the four values below to match your WCB system. Find them via the
  WCB Config Tool or by querying a WCB over serial (?WCBM, ?WCBP, ?WCBQ).
*/

#include <WCBClient.h>
#include <WCBStream.h>
#include <PololuMaestro.h>  // Pololu Maestro library by Pololu

// ─────────────────────────────────────────────────────────────────────────────
// Network credentials — must match your WCB system exactly
// ─────────────────────────────────────────────────────────────────────────────
const uint8_t MAC_OCT2     = 0x00;
const uint8_t MAC_OCT3     = 0x00;
const char*   PASSWORD     = "change_me_or_risk_takeover";
const uint8_t WCB_QUANTITY = 4;
const uint8_t DEVICE_ID    = 20;  // 20 = special out-of-band slot

// ─────────────────────────────────────────────────────────────────────────────
// Maestro device number
//
// Set this to match the device number configured in Maestro Control Center
// (Serial Settings tab). This enables the Pololu protocol so each packet
// includes the 0xAA start byte and the device number — required when your
// show controller addresses Maestros by number.
//
// Use Maestro::deviceNumberDefault (255) for the compact protocol instead.
// ─────────────────────────────────────────────────────────────────────────────
const uint8_t MAESTRO_DEVICE = 1;

// ─────────────────────────────────────────────────────────────────────────────
// WCBClient and WCBStream — broadcast mode
//
// WCBClient must be declared BEFORE WCBStream. Passing  broadcast  as the
// target tells WCBStream to use sendKyber() — one ESP-NOW packet reaches
// every WCB; those with Kyber_Remote forward the bytes to their Maestro.
// ─────────────────────────────────────────────────────────────────────────────
WCBClient wcb(MAC_OCT2, MAC_OCT3, PASSWORD, WCB_QUANTITY, DEVICE_ID);

WCBStream maestroStream(broadcast);   // broadcast to all WCBs with Maestros

// Pass WCBStream to the Pololu library instead of a hardware serial port.
// The Pololu library writes bytes here; WCBStream forwards them wirelessly.
MiniMaestro maestro(maestroStream, Maestro::noResetPin, MAESTRO_DEVICE);

unsigned long lastMoveMs = 0;

void setup() {
    Serial.begin(115200);
    wcb.begin();
}

void loop() {
    // update() drives heartbeats and flushes WCBStream automatically.
    wcb.update();

    // Sweep servo channel 0 back and forth every 2 seconds.
    // The same command reaches every WCB with a Maestro simultaneously.
    if (millis() - lastMoveMs >= 2000) {
        lastMoveMs = millis();
        static bool toggle = false;
        toggle = !toggle;

        // Quarter-microseconds: 4000=1000µs  6000=1500µs  8000=2000µs
        uint16_t pos = toggle ? 4000 : 8000;
        maestro.setTarget(0, pos);
        Serial.printf("[TX] Broadcast Maestro ch0 → %d (%s)\n",
                      pos, toggle ? "left" : "right");
    }
}
