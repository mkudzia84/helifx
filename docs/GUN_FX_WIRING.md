# Gun FX Wiring Guide

## Overview

The gun FX system provides realistic helicopter weapon effects:
- Multiple rates of fire (selectable via PWM)
- Nozzle flash LED (blinks at firing rate)
- Audio playback (looping gun sounds)
- Smoke generator (fan + heater)
- Turret servo control (pitch, yaw)
- Status LEDs (heater indicator, firing/status)

## System Architecture

The gun FX system uses a distributed architecture:

```
RC Receiver ──► Raspberry Pi (Hub) ◄──USB CDC──► Raspberry Pi Pico (GunFX)
                     │                                    │
                     │ Audio output                       ├─► Servos (1, 2, 3)
                     ▼                                    ├─► Nozzle Flash LED
                 Speakers                                 ├─► Smoke Fan
                                                          ├─► Smoke Heater
                                                          ├─► Blue Status LED
                                                          └─► Yellow Heater LED
```

**Raspberry Pi (Hub):**
- Reads PWM inputs from RC receiver
- Plays audio (engine and gun sounds)
- Sends commands to Pico over USB serial

**Raspberry Pi Pico (GunFX Controller):**
- Controls all gun hardware outputs
- Servo motion profiling (smooth accel/decel)
- Muzzle flash timing (synced to firing rate)
- Smoke control (heater + fan with delayed off)
- Status LEDs (heater indicator, firing status)
- Watchdog timeout (safe shutdown if no signal)

## Pico Pinout

| GPIO | Function |
|------|----------|
| 1 | Servo 1 |
| 2 | Servo 2 |
| 3 | Servo 3 |
| 13 | Blue LED (status) |
| 14 | Yellow LED (heater indicator) |
| 16 | Smoke Fan Motor |
| 17 | Smoke Heater |
| 25 | Nozzle Flash (PWM) |

## Wiring Diagrams

### Servo Wiring

```
Pico                         Servo
────                         ─────
GPIO 1 ──────────────────►   Signal (white/yellow)
                             Power (red) ◄──── External 5-6V
GND ─────────────────────►   Ground (black/brown)
```

**Notes:**
- Use external power for servos (not from Pico)
- Share common ground between Pico and servo power supply
- Servos typically need 5-6V, 500mA+ each

### Nozzle Flash LED

```
Pico                    LED Circuit
────                    ───────────
GPIO 25 ────[ 220Ω ]────►├ LED ├─── GND
```

### Smoke Generator (MOSFET Control)

```
Pico               MOSFET Module           Smoke Component
────               ──────────────           ───────────────
GPIO 16 ──────────►  Signal (Fan)
GPIO 17 ──────────►  Signal (Heater)
GND ──────────────►  GND

External 12V+ ────►  V+ ──────────────────► Fan/Heater (+)
External GND ─────►  GND ─────────────────► Fan/Heater (-)
```

**Recommended MOSFET modules:** IRF520, IRLZ44N (logic-level)

### Status LEDs

```
Pico                    LED Circuits
────                    ────────────
GPIO 13 ────[ 220Ω ]────►├ Blue LED ├─── GND    (Status)
GPIO 14 ────[ 220Ω ]────►├ Yellow LED ├── GND   (Heater indicator)
```

**LED Behavior:**
- **Yellow LED:** Solid ON when heater is active
- **Blue LED:** 
  - OFF when idle
  - Synced with muzzle flash when firing
  - Slow blink (1s on / 2s off) on watchdog timeout

## USB Connection

Connect Pico to Raspberry Pi via USB cable. The Pico enumerates as:
- **VID:** 0x2e8a (Raspberry Pi Foundation)
- **PID:** 0x0180 (gunfx_pico)

The hub auto-detects the device by VID/PID - no configuration required.

## Power Requirements

| Component | Voltage | Current | Notes |
|-----------|---------|---------|-------|
| Pico | 5V (USB) | 50mA | Powered from Pi USB |
| Servos | 5-6V | 500mA each | External supply |
| Smoke Fan | 12V | 500mA | External supply |
| Smoke Heater | 12V | 1-3A | External supply |
| LEDs | 3.3V | 10mA each | From Pico GPIO |

**Important:**
- Use separate power supplies for high-current loads
- Share common ground between all power supplies and Pico
- Add flyback diodes on inductive loads (fan motor)

## Safety Notes

⚠️ **Important Safety Information:**

1. **Never power smoke generator from Pico GPIO**
2. **Use MOSFET modules** for smoke fan and heater control
3. **Smoke heaters get hot** - mount in ventilated area
4. **Add inline fuse** (5A) on 12V smoke generator supply
5. **Test components individually** before full integration
6. **Ensure adequate wire gauge** for high-current paths (16-18 AWG minimum)
