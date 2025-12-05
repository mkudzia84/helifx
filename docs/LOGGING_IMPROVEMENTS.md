# Logging Improvements Summary

## Overview
Comprehensive refactoring and enhancement of the logging system across all HeliFX components for better debugging, monitoring, and maintenance.

## Changes Made

### 1. **New Logging Header (`include/logging.h`)**
- Created unified logging macros for consistent output across all components
- Implemented standardized component prefixes (LOG_HELIFX, LOG_GUN, LOG_ENGINE, etc.)
- Defined four log levels: ERROR, WARN, INFO, DEBUG
- Added specialized macros: LOG_STATE, LOG_INIT, LOG_SHUTDOWN, LOG_STATUS
- Enabled future color coding support for terminal output

**Key Features:**
- Component-based tagging for easy filtering
- Standardized format: `[COMPONENT] Level: Message`
- Compile-time debug logging (with `-DDEBUG` flag)
- Consistent parameter formatting (units, precision)

### 2. **Component Integration**
Added logging.h includes to:
- `src/gun_fx.c` - Gun effects controller
- `src/engine_fx.c` - Engine sound effects
- `src/servo.c` - Servo motor control
- `src/audio_player.c` - Audio mixer and playback
- `src/config_loader.c` - Configuration parser
- `src/main.c` - Main application

### 3. **Logging Documentation (`docs/LOGGING.md`)**
Comprehensive guide covering:
- Log levels and when to use each
- Component prefixes and categorization
- Specialized macro usage examples
- Best practices for consistent logging
- Log filtering techniques (journalctl, grep)
- Example output from startup to operation
- Troubleshooting guide

### 4. **Code Examples and Usage**
Added real-world examples for:
- Gun FX status logging (periodic 10-second dumps)
- State transitions (IDLE → FIRING, STOPPED → STARTING)
- Error conditions and error handling
- Component initialization and shutdown
- Debug-level detailed diagnostic information

## Benefits

### For Developers
- **Consistency:** All components follow same logging pattern
- **Filtering:** Easy to focus on specific components or log levels
- **Maintainability:** Clear understanding of system flow via logs
- **Debugging:** Structured output makes troubleshooting faster

### For Operations
- **Monitoring:** Clear status messages at regular intervals
- **Diagnostics:** Comprehensive error and warning information
- **Analysis:** Timestamped events in journalctl/syslog
- **Filtering:** Standard component names for automated analysis

### For Users
- **Transparency:** Clear feedback about system operation
- **Troubleshooting:** Helpful error messages with context
- **Documentation:** Log output serves as implicit system documentation

## Usage Examples

### Starting the System
```
[CONFIG] System Configuration
[ENGINE] PWM monitoring started on pin 17 (threshold: 1500 us)
[ENGINE] Engine FX created (channel: 0)
[GUN]    Nozzle flash LED initialized on GPIO 23
[GUN]    Smoke generator initialized (heater GPIO 25, fan GPIO 24)
[GUN]    Trigger PWM monitoring started on GPIO 27
[HELIFX] System ready. Press Ctrl+C to exit.
```

### Runtime Operation
```
[ENGINE] State: STOPPED → STARTING
[ENGINE] Playing starting sound
[GUN]    State: IDLE → FIRING
[GUN]    Firing started (550 RPM)
[GUN STATUS @ 10.2s] Firing: YES | Rate: 2 | RPM: 550 | Trigger PWM: 1650 µs | Heater: ON
[GUN SERVOS] Pitch: 1500 µs | Yaw: 1600 µs | Pitch Servo: ACTIVE | Yaw Servo: ACTIVE
```

### Filtering Examples
```bash
# View only gun-related messages
sudo journalctl -u helifx | grep "\[GUN\]"

# View only errors and warnings
sudo journalctl -u helifx | grep -E "(Error:|Warning:)"

# View periodic status dumps
sudo journalctl -u helifx | grep "STATUS"
```

## Enhanced Logging Points

### Gun FX Component
- ✓ Initialization with component details (nozzle LED, smoke, PWM monitors, servos)
- ✓ Processing thread start/stop
- ✓ Rate of fire changes with RPM values
- ✓ Firing state transitions (IDLE ↔ FIRING)
- ✓ Smoke heater/fan control state changes
- ✓ Periodic 10-second status dumps with servo information
- ✓ Error handling with descriptive messages

### Engine FX Component
- ✓ PWM monitoring on startup with threshold values
- ✓ Engine state transitions (STOPPED → STARTING → RUNNING → STOPPING)
- ✓ Sound file playback tracking
- ✓ Transition timing information
- ✓ Thread lifecycle management

### Servo Component
- ✓ Configuration details (input/output ranges, speed, acceleration)
- ✓ Servo creation with all parameters
- ✓ Error conditions with validation info
- ✓ Thread lifecycle

### Audio System
- ✓ Sound file loading with filename
- ✓ Channel allocation and playback tracking
- ✓ Mixer creation with channel count
- ✓ Error handling for missing files

## Backward Compatibility

All existing logging statements remain functional. The new logging macros are additions that gradually replace manual printf/fprintf calls. The system maintains full backward compatibility while providing a migration path.

## Future Enhancements

Potential improvements for future versions:
1. **Log Rotation:** Implement file-based logging with rotation
2. **Structured Logging:** JSON format for automated parsing
3. **Performance Metrics:** Log timing information for optimization
4. **Remote Logging:** Send logs to syslog server or cloud service
5. **Log Levels:** Runtime configuration of log verbosity
6. **Colored Output:** Enable/disable colored terminal output
7. **Trace Logging:** Call stack traces for debugging

## Related Files

- `include/logging.h` - Logging macro definitions
- `docs/LOGGING.md` - Comprehensive logging documentation
- `docs/README.md` - Updated with logging reference
- All `src/*.c` files - Updated with logging.h include

## Testing

To verify logging works correctly:

1. **Build with standard flags:**
   ```bash
   make clean && make
   ```

2. **Build with debug logging:**
   ```bash
   make clean
   make CFLAGS="-Wall -Wextra -Werror -Wno-unused-function -std=c11 -pthread -O2 -D_DEFAULT_SOURCE -DDEBUG"
   ```

3. **Run and observe output:**
   ```bash
   ./build/helifx config.yaml
   ```

4. **Filter specific components:**
   ```bash
   ./build/helifx config.yaml 2>&1 | grep "\[GUN\]"
   ```

## Conclusion

The logging system improvements provide:
- **Consistency** across all components
- **Clarity** in system operation and diagnostics
- **Flexibility** for future enhancements
- **Developer Experience** improvements for debugging
- **Operational Visibility** for monitoring and maintenance
