# GunFX Controller - Raspberry Pi Pico

Slave microcontroller for gun FX hardware control. Receives commands from the main Raspberry Pi over USB serial and drives:
- Nozzle flash LED (PWM)
- Smoke heater and fan
- Turret servos (pitch, yaw, retract)

## Hardware Setup

### Raspberry Pi Pico Pins

Outputs are grouped on the **right side** with servos adjacent:

**Gun Servo Outputs (PWM):**
- GPIO 19 (Physical Pin 28): Gun Servo 1
- GPIO 20 (Physical Pin 30): Gun Servo 2
- GPIO 21 (Physical Pin 32): Gun Servo 3

**Smoke and Flash Outputs:**
- GPIO 22 (Physical Pin 34): Nozzle flash LED (PWM)
- GPIO 16 (Physical Pin 21): Smoke heater (MOSFET/relay)
- GPIO 17 (Physical Pin 22): Smoke fan (MOSFET/relay)

**Board Layout Reference:**
```
Left Side (1-20)     |     Right Side (21-40)
─────────────────────────────────────────
...                  |     GP16 · GP17 (21-22) ← Heater / Fan
...                  |     GND  ·  GP19 (28)    ← Servo 1
...                  |     GND  ·  GP20 (30)    ← Servo 2
...                  |     GND  ·  GP21 (32)    ← Servo 3
...                  |     GND  ·  GP22 (34)    ← Flash LED
...                  |     ...  ·  ...
```

**Power:**
- Connect Pico via USB to main Raspberry Pi
- Use external power supply for servos and high-current loads (smoke heater/fan)
- Common ground between Pico, power supply, and all peripherals

**MOSFET Drivers:**
- For smoke heater and fan, use logic-level MOSFETs (e.g., IRLZ44N)
- Add flyback diodes if loads are inductive
- Gate resistors: 220Ω–1kΩ between Pico GPIO and MOSFET gate

## Serial Protocol

**Baud Rate:** 115200  
**Format:** ASCII commands terminated by newline (`\n`)

### Commands

| Command | Description | Example |
|---------|-------------|---------|
| `TRIGGER_ON;rpm` | Start firing at specified RPM | `TRIGGER_ON;400` |
| `TRIGGER_OFF;delay_ms` | Stop firing; delay before fan turns off | `TRIGGER_OFF;3000` |
| `SRV:servo_id:pulse_us` | Set servo position with per-servo limits (300-2700 µs clamp) | `SRV:1:1500` |
| `SRV_SETTINGS:servo_id:min:max:max_speed:accel:decel` | Configure servo limits and stored motion profile values (max_speed: µs/s, accel/decel: µs/s²) | `SRV_SETTINGS:1:800:2200:4000:8000:8000` |
| `SMOKE_HEAT_ON` | Turn smoke heater on | `SMOKE_HEAT_ON` |
| `SMOKE_HEAT_OFF` | Turn smoke heater off | `SMOKE_HEAT_OFF` |

**Servo motion:** `SRV` commands move smoothly using trapezoidal velocity: accel until max_speed, decel to stop at target, and automatic braking/turnaround when reversing direction. Defaults: max_speed=4000 µs/s, accel=8000 µs/s², decel=8000 µs/s²; override per servo with `SRV_SETTINGS`.

### Firing Behavior

- `TRIGGER_ON` starts the muzzle flash blinking at the specified RPM
- Flash pulse duration: 30 ms per shot
- Smoke fan turns on immediately when firing starts
- When `TRIGGER_OFF` is received, the muzzle flash stops immediately, but the smoke fan continues running for the specified delay (e.g., 3000 ms) before turning off

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

### Using PlatformIO (Optional)

Create `platformio.ini` in the `pico` folder:

```ini
[env:pico]
platform = raspberrypi
board = pico
framework = arduino
board_build.core = earlephilhower
lib_deps = 
    Servo
monitor_speed = 115200
```

Then:
```bash
cd controllers/gunfx/pico
pio run -t upload
pio device monitor
```

## Testing

### Serial Monitor Test

Open Serial Monitor at 115200 baud and send commands:

```
SMOKE_HEAT_ON
TRIGGER_ON;200
SRV:1:1500
SRV:2:1500
SRV:3:1500
TRIGGER_OFF;3000
SMOKE_HEAT_OFF
```

Expected output:
- LED flashes at specified rate
- Servos move to commanded positions with smooth accel/decel and clamped limits
- Heater/fan respond to commands
- Status messages confirm each command

### Python Test Script (from main Raspberry Pi)

```python
import serial
import time

# Open serial connection to Pico
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
time.sleep(2)  # Wait for Pico to initialize

# Turn heater on
ser.write(b'SMOKE_HEAT_ON\n')
time.sleep(0.5)

# Start firing at 400 RPM
ser.write(b'TRIGGER_ON;400\n')
time.sleep(5)

# Set gun servo positions
ser.write(b'SRV:1:1600\n')
ser.write(b'SRV:2:1400\n')
ser.write(b'SRV:3:1500\n')
time.sleep(2)

# Stop firing with 3 second fan delay
ser.write(b'TRIGGER_OFF;3000\n')
time.sleep(5)

# Turn heater off
ser.write(b'SMOKE_HEAT_OFF\n')

# Read responses
while ser.in_waiting:
    print(ser.readline().decode().strip())

ser.close()
```

## Pin Customization

To change pin assignments, edit the constants at the top of `gunfx_pico.ino`. The format is:

```cpp
const uint8_t PIN_NAME = GPIO; // GP[GPIO] (Physical Pin [PIN])
```

Current configuration (right side grouping):
```cpp
// Gun Servo Outputs (PWM)
const uint8_t PIN_GUN_SRV_1       = 19; // GP19 (Physical Pin 28)
const uint8_t PIN_GUN_SRV_2       = 20; // GP20 (Physical Pin 30)
const uint8_t PIN_GUN_SRV_3       = 21; // GP21 (Physical Pin 32)

// Smoke and Flash Outputs
const uint8_t PIN_NOZZLE_FLASH    = 22; // GP22 (Physical Pin 34)
const uint8_t PIN_SMOKE_HEATER    = 16; // GP16 (Physical Pin 21)
const uint8_t PIN_SMOKE_FAN       = 17; // GP17 (Physical Pin 22)
```

All Pico GPIOs (0-25) support PWM and can be used for servo control or LED PWM.

## Troubleshooting

**Pico not detected:**
- Hold BOOTSEL button while connecting USB
- Pico should appear as a mass storage device
- Drag and drop UF2 file from Arduino IDE build output

**Servos jittering:**
- Check power supply voltage (servos typically need 5-6V)
- Ensure common ground between Pico and servo power
- Add decoupling capacitors near servos

**High-current loads not working:**
- Verify MOSFET wiring and gate resistors
- Check power supply capacity
- Use multimeter to verify GPIO output voltage (3.3V)

**Serial communication issues:**
- Confirm baud rate (115200)
- Check USB cable (must support data, not just power)
- On Linux, add user to `dialout` group: `sudo usermod -a -G dialout $USER`
