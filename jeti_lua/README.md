# HeliFX Jeti Lua Scripts

Lua applications for Jeti transmitters to remotely configure and monitor HeliFX helicopter effects system via JetiEX telemetry.

## Files

- `helifx_config.lua` - Main configuration application
- `helifx_config.jsn` - Application metadata and settings
- `README.md` - This file

## Installation

### Method 1: SD Card

1. Insert transmitter SD card into computer
2. Navigate to `/Apps/` directory
3. Create folder `HeliFX/`
4. Copy `helifx_config.lua` and `helifx_config.jsn` to `/Apps/HeliFX/`
5. Safely eject SD card and insert back into transmitter
6. Restart transmitter

### Method 2: Jeti Studio

1. Connect transmitter to computer via USB
2. Open Jeti Studio software
3. Navigate to Applications tab
4. Click "Install Application"
5. Select `helifx_config.lua` and `helifx_config.jsn`
6. Upload to transmitter

## Configuration

### First Time Setup

1. **Load Application:**
   - Press `Menu` → `Applications` → `HeliFX Config`

2. **Configure Sensors:**
   - Press `Menu` → `Applications` → `HeliFX Config` → Settings (gear icon)
   - Select telemetry sensors for:
     - Gun RPM
     - Engine RPM
     - Battery Voltage
     - Temperature
   - Set Device ID (default: 42505 / 0xA409)

3. **Verify Connection:**
   - Open application
   - Telemetry values should appear in telemetry window
   - Status should show "Ready"

## Usage

### Main Screen

The main configuration screen displays all adjustable HeliFX parameters:

```
┌────────────────────────────────────┐
│ HeliFX Config          Ready       │
├────────────────────────────────────┤
│ > Gun Rate 1 RPM           200     │
│   Gun Rate 2 RPM           550     │
│   Gun Rate 1 PWM          1400     │
│   Gun Rate 2 PWM          1600     │
│   Smoke Fan Delay         2000     │
│   Heater PWM Thresh       1500     │
│   Engine PWM Thresh       1500     │
│   Servo Max Speed          500     │
│   Servo Max Accel         2000     │
│   Telemetry Rate Hz         10     │
│   Nozzle Flash Enable       ON     │
│   Smoke Enable              ON     │
├────────────────────────────────────┤
│ F1:Read  F2:Write  F3:Save All    │
└────────────────────────────────────┘
```

### Controls

| Key | Function | Description |
|-----|----------|-------------|
| **↑/↓** | Navigate | Select parameter (menu mode) |
| **↑/↓** | Adjust | Change value (edit mode) |
| **ENTER** | Edit/Confirm | Toggle edit mode |
| **F1** | Read | Read current value from HeliFX |
| **F2** | Write | Write current value to HeliFX |
| **F3** | Save All | Save all parameters to HeliFX config.yaml |
| **ESC** | Cancel | Exit edit mode |

### Workflow

#### Reading Parameters from HeliFX

1. Navigate to desired parameter with ↑/↓
2. Press **F1** to request current value from HeliFX
3. Wait for value to update (status shows "Reading param...")
4. Value is automatically updated in list

#### Writing Parameters to HeliFX

1. Navigate to desired parameter with ↑/↓
2. Press **ENTER** to enter edit mode (parameter highlighted)
3. Use ↑/↓ to adjust value
4. Press **ENTER** to confirm (or **ESC** to cancel)
5. Press **F2** to write value to HeliFX
6. Value is applied immediately and saved to transmitter

#### Saving Configuration

1. Adjust multiple parameters as needed
2. Press **F3** to save entire configuration to HeliFX
3. HeliFX writes changes to `config.yaml`
4. Status shows "Configuration saved!" on success

### Telemetry Window

The telemetry window shows real-time values from HeliFX:

```
┌──────────────────┐
│ Gun:    550 RPM  │
│ Engine: 1500 RPM │
│ Battery: 12.6V   │
│ Temp:    45°C    │
└──────────────────┘
```

Add this window to your telemetry screen:
1. `Menu` → `Timers/Sensors` → `Displayed Telemetry`
2. Select empty slot
3. Choose `HeliFX Config`
4. Position and size as desired

## Parameters

### Gun Configuration

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| Gun Rate 1 RPM | uint16 | 0-1000 | Low rate of fire |
| Gun Rate 2 RPM | uint16 | 0-1000 | High rate of fire |
| Gun Rate 1 PWM | uint16 | 1000-2000 | PWM threshold for rate 1 |
| Gun Rate 2 PWM | uint16 | 1000-2000 | PWM threshold for rate 2 |

### Smoke Configuration

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| Smoke Fan Delay | uint16 | 0-5000 | Fan off delay in ms |
| Heater PWM Thresh | uint16 | 1000-2000 | Heater toggle threshold |
| Smoke Enable | bool | ON/OFF | Enable smoke generator |

### Servo Configuration

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| Servo Max Speed | uint16 | 0-2000 | Max servo speed (µs/sec) |
| Servo Max Accel | uint16 | 0-5000 | Max servo accel (µs/sec²) |

### System Configuration

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| Engine PWM Thresh | uint16 | 1000-2000 | Engine on/off threshold |
| Telemetry Rate Hz | uint8 | 5-100 | Telemetry update rate |
| Nozzle Flash Enable | bool | ON/OFF | Enable LED flash |

## Troubleshooting

### No Telemetry Data

**Symptoms:** Telemetry window shows "---" for all values

**Solutions:**
1. Check HeliFX telemetry is enabled in `config.yaml`:
   ```yaml
   jetiex:
     enabled: true
   ```
2. Verify serial connection (GPIO 14 to EX Bus)
3. Check sensor binding in transmitter settings
4. Restart HeliFX and transmitter

### Cannot Write Parameters

**Symptoms:** Pressing F2 has no effect, status shows errors

**Solutions:**
1. Ensure HeliFX remote configuration is enabled:
   ```yaml
   jetiex:
     remote_config: true
   ```
2. Check bidirectional communication (UART configured properly)
3. Verify Device ID matches between Lua app and HeliFX
4. Check HeliFX logs with `make debug`

### Values Revert After Save

**Symptoms:** Parameters reset to old values after saving

**Solutions:**
1. Check HeliFX has write permission to `config.yaml`
2. Ensure config save callback is registered in HeliFX code
3. Verify `config.yaml` format is correct
4. Check HeliFX logs for save errors

### Lua App Not Loading

**Symptoms:** Application doesn't appear in menu

**Solutions:**
1. Check files are in correct location: `/Apps/HeliFX/`
2. Verify both `.lua` and `.jsn` files are present
3. Check file names match exactly (case-sensitive)
4. Restart transmitter
5. Try reinstalling via Jeti Studio

## Advanced Usage

### Adjusting Parameters In-Flight

**WARNING:** Use caution when adjusting parameters during flight!

Safe to adjust in-flight:
- ✅ Gun rate of fire (within reasonable limits)
- ✅ Smoke fan delay
- ✅ Telemetry update rate

**NOT** recommended in-flight:
- ❌ Servo parameters (may cause sudden movements)
- ❌ PWM thresholds (may cause unexpected state changes)
- ❌ Enable/disable features

### Creating Custom Parameter Presets

You can create multiple saved presets by:
1. Adjust all parameters for specific scenario
2. Write all values to HeliFX (F2 for each)
3. Save configuration (F3)
4. Note parameter values in transmitter notepad
5. Repeat for different scenarios
6. Quickly switch by loading preset values and writing

### Integration with Jeti Switches

Assign specific parameter writes to transmitter switches:
1. Create Lua script for specific parameter
2. Bind to switch position
3. Automatically adjusts when switch toggled

Example: Toggle smoke on/off with SA switch

## Protocol Details

### Packet Format

**Read Request:**
```
[0x3B][MFR_L][MFR_H][DEV_L][DEV_H][0x01][PARAM_ID][CRC_L][CRC_H]
```

**Write Command:**
```
[0x3B][MFR_L][MFR_H][DEV_L][DEV_H][0x02][PARAM_ID][VALUE...][CRC_L][CRC_H]
```

**Save Command:**
```
[0x3B][MFR_L][MFR_H][DEV_L][DEV_H][0x04][0x00][CRC_L][CRC_H]
```

### CRC Calculation

CRC-16 CCITT (polynomial 0x1021, initial value 0x0000)

## Version History

### v1.0 (2025-12-05)
- Initial release
- Remote parameter read/write
- Configuration save functionality
- Telemetry display
- 12 configurable parameters

## Support

For issues or questions:
1. Check HeliFX documentation: `docs/JETIEX.md`
2. Review HeliFX logs with debug build
3. Test with `jetiex_demo` first
4. Open issue on project repository

## License

Same license as HeliFX project.
