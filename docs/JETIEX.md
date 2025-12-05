# JetiEX Telemetry Integration

## Overview

The HeliFX system now supports **JetiEX telemetry protocol** for transmitting real-time data to Jeti receivers and transmitters. This enables monitoring of gun firing rates, engine status, battery voltage, temperature, and other parameters directly on your RC transmitter display.

## Features

- **Multiple sensor types**: RPM, voltage, current, temperature, percentage
- **Configurable update rate**: 5-100 Hz telemetry transmission
- **Text messages**: Send status messages to transmitter display
- **Efficient protocol**: Standard JetiEX Bus protocol with CRC validation
- **Thread-based**: Non-blocking telemetry transmission
- **Flexible configuration**: Enable/disable individual sensors via config.yaml

## Hardware Requirements

### UART Serial Port

JetiEX telemetry requires a free UART serial port on the Raspberry Pi:

| Option | Port | Notes |
|--------|------|-------|
| **Primary UART** | `/dev/ttyAMA0` | GPIO 14/15 (pins 8/10) - **Recommended** |
| **Secondary UART** | `/dev/ttyS0` | Mini UART - Lower performance |
| **USB Serial** | `/dev/ttyUSB0` | USB-to-serial adapter |

### Wiring

**Connection to Jeti Receiver:**

```
Raspberry Pi                    Jeti Receiver
------------                    -------------

GPIO 14 (TXD) ----[1kΩ]----->  EX Bus (Signal)
GPIO 15 (RXD) --------------->  Not connected
     GND     ----------------->  GND (Common)
```

**Important:**
- Use a 1kΩ resistor in series with TX line for protection
- JetiEX is a half-duplex protocol (transmit only from Pi perspective)
- Ensure common ground between Pi and receiver
- 3.3V logic levels compatible with Jeti receivers

### Enabling UART on Raspberry Pi

Edit `/boot/config.txt`:

```bash
# Disable Bluetooth to free up primary UART
dtoverlay=disable-bt

# Enable UART
enable_uart=1
```

Disable serial console (if enabled):
```bash
sudo raspi-config
# Interface Options -> Serial Port
# Login shell: No
# Serial hardware: Yes
```

Reboot after changes:
```bash
sudo reboot
```

## Configuration

### config.yaml

Enable and configure JetiEX telemetry:

```yaml
jetiex:
  enabled: true                # Enable JetiEX telemetry
  serial_port: "/dev/ttyAMA0"  # Serial port for telemetry
  baud_rate: 115200            # Baud rate (9600-230400)
  manufacturer_id: 0xA409      # Manufacturer ID (16-bit hex)
  device_id: 0x0001            # Device ID (16-bit hex)
  update_rate_hz: 10           # Telemetry update rate (5-100 Hz)
  text_messages: true          # Enable text messages
  
  sensors:
    gun_rpm:
      enabled: true            # Gun firing rate in RPM
      sensor_id: 0
      label: "Gun RPM"
    
    engine_rpm:
      enabled: true            # Engine RPM (if available)
      sensor_id: 1
      label: "Engine RPM"
    
    battery_voltage:
      enabled: true            # Battery voltage monitoring
      sensor_id: 2
      label: "Battery"
      precision: 1             # Decimal places (0-2)
    
    temperature:
      enabled: true            # System temperature
      sensor_id: 3
      label: "Temperature"
      precision: 0
    
    ammunition:
      enabled: false           # Simulated ammunition level
      sensor_id: 4
      label: "Ammunition"
```

### Parameter Details

| Parameter | Values | Description |
|-----------|--------|-------------|
| `enabled` | true/false | Enable/disable telemetry subsystem |
| `serial_port` | `/dev/ttyAMA0`, etc. | UART device path |
| `baud_rate` | 9600-230400 | Serial communication speed |
| `manufacturer_id` | 0x0000-0xFFFF | 16-bit manufacturer identifier |
| `device_id` | 0x0000-0xFFFF | 16-bit device identifier |
| `update_rate_hz` | 5-100 | Telemetry packets per second |
| `text_messages` | true/false | Enable status text messages |

**Sensor Configuration:**

| Parameter | Description |
|-----------|-------------|
| `enabled` | Enable/disable individual sensor |
| `sensor_id` | Unique sensor ID (0-15) |
| `label` | Display name on transmitter (max 20 chars) |
| `precision` | Decimal places for voltage/temp (0-2) |

## Available Sensors

### Gun RPM
- **ID**: 0
- **Type**: 22-bit integer
- **Range**: 0 - 2,097,151 RPM
- **Description**: Current gun firing rate in rounds per minute

### Engine RPM
- **ID**: 1
- **Type**: 22-bit integer
- **Range**: 0 - 2,097,151 RPM
- **Description**: Engine speed (if available from throttle input)

### Battery Voltage
- **ID**: 2
- **Type**: 14-bit integer
- **Range**: 0.0 - 819.1V
- **Precision**: 0-2 decimal places
- **Description**: System battery voltage

### Temperature
- **ID**: 3
- **Type**: 14-bit integer
- **Range**: -819.1 to +819.1°C
- **Precision**: 0-1 decimal places
- **Description**: System or component temperature

### Ammunition
- **ID**: 4
- **Type**: 14-bit integer
- **Range**: 0 - 100%
- **Description**: Simulated ammunition remaining

## Usage

### Demo Program

Test JetiEX telemetry with simulated data:

```bash
# Build demo
make demo

# Run with defaults
./build/jetiex_demo

# Run with custom settings
./build/jetiex_demo --serial=/dev/ttyUSB0 --baud=9600 --rate=20

# Show help
./build/jetiex_demo --help
```

**Demo Features:**
- Simulates all 5 sensors with realistic data
- Gun alternates firing/idle every 5 seconds
- Engine RPM varies randomly
- Battery slowly discharges
- Temperature increases when firing
- Ammunition depletes and reloads
- Sends text messages for major events

### Integration Example

```c
#include "jetiex.h"

// Configuration
JetiEXConfig config = {
    .serial_port = "/dev/ttyAMA0",
    .baud_rate = 115200,
    .manufacturer_id = 0xA409,
    .device_id = 0x0001,
    .update_rate_hz = 10,
    .text_messages = true
};

// Create telemetry
JetiEX *jetiex = jetiex_create(&config);

// Add sensors
JetiEXSensor gun_rpm = jetiex_sensor_rpm(0, "Gun RPM");
jetiex_add_sensor(jetiex, &gun_rpm);

JetiEXSensor voltage = jetiex_sensor_voltage(2, "Battery", 1);
jetiex_add_sensor(jetiex, &voltage);

// Start telemetry
jetiex_start(jetiex);

// Update sensor values during operation
jetiex_update_sensor(jetiex, 0, 550);  // Gun firing at 550 RPM
jetiex_update_sensor(jetiex, 2, 126);  // Battery at 12.6V (precision 1)

// Send text message
jetiex_send_text(jetiex, "Gun Firing!");

// Cleanup
jetiex_stop(jetiex);
jetiex_destroy(jetiex);
```

## Receiver Setup

### Jeti Receiver Configuration

1. **Connect telemetry wire** from receiver EX Bus to Raspberry Pi TX (GPIO 14)
2. **Power on system** and verify telemetry LED on receiver blinks
3. **Access receiver menu** on transmitter
4. **Auto-detect sensors** (receiver should find HeliFX device)
5. **Assign sensors to telemetry screens** on transmitter

### Typical Receiver Settings

- **Protocol**: EX Bus
- **Update Rate**: Matches config.yaml `update_rate_hz`
- **Device**: Should show manufacturer ID 0xA409 (or your custom ID)

### Transmitter Display

Sensors will appear with configured labels:
- Gun RPM
- Engine RPM
- Battery
- Temperature
- Ammunition

Text messages appear in telemetry message area when events occur.

## Protocol Specification

### Packet Format

**Telemetry Data Packet:**
```
Byte 0:    0x3A (Packet type - Data)
Byte 1-2:  Manufacturer ID (16-bit, little-endian)
Byte 3-4:  Device ID (16-bit, little-endian)
Byte 5:    0x00 (Reserved)
Byte 6:    0x3A (Data section marker)
Byte 7-N:  Sensor data (variable length)
Byte N+1-N+2: CRC-16 CCITT (little-endian)
```

**Text Message Packet:**
```
Byte 0:    0xA4 (Packet type - Message)
Byte 1-2:  Manufacturer ID
Byte 3-4:  Device ID
Byte 5:    Text length
Byte 6-N:  Text message (ASCII, max 32 chars)
Byte N+1-N+2: CRC-16 CCITT
```

### Data Encoding

Each sensor occupies 2-5 bytes depending on data type:

| Type | Bits | Range | Bytes |
|------|------|-------|-------|
| 6b   | 6    | ±31 | 2 |
| 14b  | 14   | ±8,191 | 3 |
| 22b  | 22   | ±2,097,151 | 4 |
| 30b  | 30   | ±536,870,911 | 5 |

## Troubleshooting

### No Telemetry on Receiver

1. **Check UART is enabled:**
   ```bash
   ls -l /dev/ttyAMA0
   # Should show: crw-rw---- 1 root dialout
   ```

2. **Verify serial port permissions:**
   ```bash
   sudo usermod -a -G dialout $USER
   # Logout and login again
   ```

3. **Test serial output:**
   ```bash
   # Run demo and monitor serial traffic
   ./build/jetiex_demo &
   sudo cat /dev/ttyAMA0 | hexdump -C
   # Should show periodic data packets
   ```

4. **Check receiver wiring:**
   - GPIO 14 (TX) connected to EX Bus
   - Common ground connected
   - 1kΩ resistor in series with TX

### Partial or Corrupt Data

1. **Reduce update rate:**
   ```yaml
   update_rate_hz: 5  # Lower rate = more reliable
   ```

2. **Try different baud rate:**
   ```yaml
   baud_rate: 9600  # More reliable than 115200
   ```

3. **Check for electrical interference:**
   - Keep telemetry wire away from power lines
   - Use shielded cable if possible
   - Add ferrite bead to telemetry wire

### High CPU Usage

1. **Reduce update rate:**
   ```yaml
   update_rate_hz: 10  # 10 Hz is typically sufficient
   ```

2. **Disable unused sensors:**
   ```yaml
   sensors:
     ammunition:
       enabled: false  # Disable sensors you don't need
   ```

## Performance Considerations

### Update Rates

| Rate | CPU Impact | Use Case |
|------|------------|----------|
| 5 Hz | Very Low | Basic monitoring |
| 10 Hz | Low | **Recommended** general use |
| 20 Hz | Medium | Fast-changing data |
| 50 Hz | High | Competition/racing |
| 100 Hz | Very High | Not recommended |

### Baud Rates

| Baud | Reliability | Latency |
|------|-------------|---------|
| 9600 | Excellent | High |
| 19200 | Very Good | Medium |
| 38400 | Good | Medium |
| 57600 | Good | Low |
| **115200** | Fair | **Very Low** (Recommended) |
| 230400 | Poor | Very Low |

## API Reference

See [`include/jetiex.h`](../include/jetiex.h) for complete API documentation.

### Key Functions

- `jetiex_create()` - Initialize telemetry
- `jetiex_destroy()` - Cleanup and close
- `jetiex_add_sensor()` - Add a sensor
- `jetiex_update_sensor()` - Update sensor value
- `jetiex_send_text()` - Send text message
- `jetiex_start()` - Start telemetry thread
- `jetiex_stop()` - Stop telemetry thread

### Helper Functions

- `jetiex_sensor_rpm()` - Create RPM sensor
- `jetiex_sensor_voltage()` - Create voltage sensor
- `jetiex_sensor_current()` - Create current sensor
- `jetiex_sensor_temperature()` - Create temperature sensor
- `jetiex_sensor_percentage()` - Create percentage sensor

## Future Enhancements

Potential additions to JetiEX integration:

- [ ] GPS coordinate telemetry
- [ ] Date/time synchronization
- [ ] Bidirectional communication (commands from transmitter)
- [ ] Alarm conditions and warnings
- [ ] Data logging to transmitter SD card
- [ ] Multi-device support on single EX Bus

## References

- [Jeti Model Official Site](https://www.jetimodel.com/)
- [EX Bus Protocol Documentation](https://www.jetimodel.com/en/telemetry/)
- [Raspberry Pi UART Configuration](https://www.raspberrypi.com/documentation/computers/configuration.html#configuring-uarts)

## Support

For issues or questions about JetiEX integration:
1. Check this documentation first
2. Review demo program source code
3. Test with `jetiex_demo` before integrating
4. Verify UART configuration on Raspberry Pi
5. Check Jeti receiver compatibility
