# GunFX Controller - Raspberry Pi Pico

Slave microcontroller for gun FX hardware control. Receives commands from the main Raspberry Pi over USB serial and drives:
- Nozzle flash LED (PWM)
- Smoke heater and fan
- Turret servos (pitch, yaw, retract)
- Status LEDs (heater indicator, firing status)

## Hardware

The GunFX board connects to the main ScaleFX Hub board via a custom USB-C cable. The USB-C cable provides both power and data communication to the Pico.

### Pico Pinout

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

### Status LED Indicators

**Yellow LED (GPIO 14):**
- **Solid ON**: Smoke heater is active
- **OFF**: Heater is off

**Blue LED (GPIO 13):**
- **OFF**: All OK (idle, normal operation)
- **Synced with nozzle flash**: Blinks at firing rate when shooting
- **Slow blink (1s on / 2s off)**: No signal from main board (watchdog timeout)

## Binary Protocol

**Baud Rate:** 115200  
**Format:** Binary packets, COBS-encoded, terminated by 0x00 delimiter

Packet format (before COBS encoding):
```
[type:u8][len:u8][payload:len bytes][crc8:u8]
```
CRC-8 polynomial 0x07 computed over type + len + payload.

### Commands (Pi → Pico)

| Type | Name | Payload | Description |
|------|------|---------|-------------|
| 0x01 | TRIGGER_ON | rpm:u16le | Start firing at specified RPM |
| 0x02 | TRIGGER_OFF | fan_delay_ms:u16le | Stop firing; delay before fan turns off |
| 0x10 | SRV_SET | servo_id:u8, pulse_us:u16le | Set servo position |
| 0x11 | SRV_SETTINGS | servo_id:u8, min:u16le, max:u16le, max_speed:u16le, accel:u16le, decel:u16le | Configure servo limits and motion profile |
| 0x12 | SRV_RECOIL_JERK | servo_id:u8, jerk_us:u16le, variance_us:u16le | Configure recoil jerk per shot |
| 0x20 | SMOKE_HEAT | on:u8 (0=off, 1=on) | Control smoke heater |
| 0xF0 | INIT | (none) | Daemon initialization - reset to safe state |
| 0xF1 | SHUTDOWN | (none) | Daemon shutdown - enter safe state |
| 0xF2 | KEEPALIVE | (none) | Periodic keepalive from daemon |

### Telemetry (Pico → Pi)

| Type | Name | Payload | Description |
|------|------|---------|-------------|
| 0xF3 | INIT_READY | module_name:string | Response to INIT with module name |
| 0xF4 | STATUS | flags:u8, fan_off_remaining_ms:u16le, servo_us[3]:u16le each, rpm:u16le | Periodic status update |

**Status flags (bit field):**
- bit0: firing
- bit1: flash_active
- bit2: flash_fading
- bit3: heater_on
- bit4: fan_on
- bit5: fan_spindown

### Servo Motion

`SRV_SET` commands move smoothly using trapezoidal velocity: accel until max_speed, decel to stop at target, and automatic braking/turnaround when reversing direction.

Defaults: max_speed=4000 µs/s, accel=8000 µs/s², decel=8000 µs/s²; override per servo with `SRV_SETTINGS`.

### Recoil Jerk Effect

The `SRV_RECOIL_JERK` command configures a simulated recoil kick effect on turret servos:

- On each shot (muzzle flash), a random jerk offset is applied to the servo position
- Jerk direction is randomly ± (positive or negative)
- Jerk magnitude = base `jerk_us` + random(0 to `variance_us`)
- The jerk is cleared after the flash fade completes

Example: With `jerk_us=50` and `variance_us=25`, each shot applies ±50µs to ±75µs of random offset.

### Firing Behavior

- `TRIGGER_ON` starts the muzzle flash blinking at the specified RPM
- Flash pulse duration: 30 ms per shot
- Smoke fan turns on immediately when firing starts
- When `TRIGGER_OFF` is received, the muzzle flash stops immediately, but the smoke fan continues running for the specified delay before turning off

## Build & Upload

### Using Arduino IDE

1. Install the [Arduino-Pico board package](https://github.com/earlephilhower/arduino-pico)
   - File → Preferences → Additional Board Manager URLs: `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
   - Tools → Board → Boards Manager → Search "pico" → Install "Raspberry Pi Pico/RP2040"

2. Open `controllers/gunfx/pico/gunfx_pico.ino`

3. Configure:
   - Tools → Board → Raspberry Pi Pico/RP2040 → Raspberry Pi Pico
   - Tools → Port → (select COM port)
   - Tools → USB Stack → "Pico SDK"

4. Upload

### Using PlatformIO

```bash
cd controllers/gunfx/pico
pio run -t upload
```

## Troubleshooting

**Pico not detected:**
- Hold BOOTSEL button while connecting USB
- Pico should appear as a mass storage device
- Drag and drop UF2 file from Arduino IDE build output

**Serial communication issues:**
- Confirm baud rate (115200)
- Check USB-C cable (must support data, not just power)
- On Linux, add user to `dialout` group: `sudo usermod -a -G dialout $USER`

