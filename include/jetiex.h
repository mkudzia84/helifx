#ifndef JETIEX_H
#define JETIEX_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file jetiex.h
 * @brief JetiEX Telemetry Protocol Implementation
 * 
 * Implements the Jeti EX Bus telemetry protocol for sending sensor data
 * to Jeti receivers and transmitters. Supports multiple sensor types and
 * text messages.
 * 
 * Protocol Specifications:
 * - Baud Rate: 125000 bps (9-bit mode) or 250000 bps (8-bit mode)
 * - Data Format: 8N1 or 9O2
 * - Max Packet Size: 29 bytes
 * - Update Rate: Typically 5-100 Hz depending on data
 */

/* Protocol Constants */
#define JETIEX_BAUD_RATE_9BIT    125000
#define JETIEX_BAUD_RATE_8BIT    250000
#define JETIEX_MAX_PACKET_SIZE   29
#define JETIEX_MAX_SENSORS       15
#define JETIEX_MAX_TEXT_LENGTH   32

/* Packet Types */
#define JETIEX_PACKET_DATA       0x3A    // Data packet identifier
#define JETIEX_PACKET_TEXT       0x00    // Text message packet
#define JETIEX_PACKET_MSG        0xA4    // Message packet
#define JETIEX_PACKET_CONFIG     0x3B    // Configuration request/response

/* Configuration Commands */
#define JETIEX_CMD_READ          0x01    // Read parameter value
#define JETIEX_CMD_WRITE         0x02    // Write parameter value
#define JETIEX_CMD_LIST          0x03    // List all parameters
#define JETIEX_CMD_SAVE          0x04    // Save configuration to file

/* Configuration Parameter Types */
#define JETIEX_PARAM_UINT8       0x01    // 8-bit unsigned integer
#define JETIEX_PARAM_UINT16      0x02    // 16-bit unsigned integer
#define JETIEX_PARAM_UINT32      0x03    // 32-bit unsigned integer
#define JETIEX_PARAM_INT8        0x04    // 8-bit signed integer
#define JETIEX_PARAM_INT16       0x05    // 16-bit signed integer
#define JETIEX_PARAM_INT32       0x06    // 32-bit signed integer
#define JETIEX_PARAM_FLOAT       0x07    // 32-bit float
#define JETIEX_PARAM_BOOL        0x08    // Boolean (0/1)
#define JETIEX_PARAM_STRING      0x09    // String (max 32 chars)

#define JETIEX_MAX_PARAMS        32      // Maximum configurable parameters
#define JETIEX_PARAM_NAME_LEN    24      // Max parameter name length

/* Data Types */
typedef enum {
    JETIEX_TYPE_6b   = 0,  // 6-bit signed (-31 to +31)
    JETIEX_TYPE_14b  = 1,  // 14-bit signed (-8191 to +8191)
    JETIEX_TYPE_22b  = 4,  // 22-bit signed (-2097151 to +2097151)
    JETIEX_TYPE_DT   = 5,  // Date/Time
    JETIEX_TYPE_30b  = 8,  // 30-bit signed (-536870911 to +536870911)
    JETIEX_TYPE_GPS  = 9   // GPS coordinates
} JetiEXDataType;

/* Sensor Units */
typedef enum {
    JETIEX_UNIT_NONE = 0,
    JETIEX_UNIT_VOLTS,       // V
    JETIEX_UNIT_AMPS,        // A
    JETIEX_UNIT_MILLIAMPS,   // mA
    JETIEX_UNIT_KMH,         // km/h
    JETIEX_UNIT_CELSIUS,     // °C
    JETIEX_UNIT_PERCENT,     // %
    JETIEX_UNIT_MAH,         // mAh
    JETIEX_UNIT_WATTS,       // W
    JETIEX_UNIT_MILLIWATTS,  // mW
    JETIEX_UNIT_DB,          // dB
    JETIEX_UNIT_RPM,         // RPM
    JETIEX_UNIT_METERS,      // m
    JETIEX_UNIT_FEET,        // ft
    JETIEX_UNIT_METERS_SEC,  // m/s
    JETIEX_UNIT_FEET_SEC,    // ft/s
    JETIEX_UNIT_MILLILITERS  // ml
} JetiEXUnit;

/* Sensor Definition */
typedef struct {
    uint8_t id;              // Sensor ID (0-15)
    char label[20];          // Sensor label (displayed on transmitter)
    char unit_label[4];      // Unit label (e.g., "rpm", "V", "°C")
    JetiEXDataType type;     // Data type
    JetiEXUnit unit;         // Unit type
    uint8_t precision;       // Decimal places (0-2)
    int32_t value;           // Current sensor value
    bool enabled;            // Whether sensor is active
} JetiEXSensor;

/* Configuration Parameter */
typedef struct {
    uint8_t id;                          // Parameter ID (0-31)
    char name[JETIEX_PARAM_NAME_LEN];    // Parameter name (for display)
    uint8_t type;                        // Parameter type (JETIEX_PARAM_*)
    void *value_ptr;                     // Pointer to actual value
    int32_t min_value;                   // Minimum allowed value
    int32_t max_value;                   // Maximum allowed value
    uint8_t flags;                       // Flags (read-only, etc.)
} JetiEXParameter;

/* Parameter Flags */
#define JETIEX_PARAM_READONLY    0x01    // Parameter is read-only
#define JETIEX_PARAM_PERSISTENT  0x02    // Parameter should be saved to config

/* Configuration Callback Types */
typedef void (*JetiEXConfigCallback)(uint8_t param_id, void *user_data);
typedef bool (*JetiEXConfigSaveCallback)(void *user_data);

/* JetiEX Configuration */
typedef struct {
    const char *serial_port; // Serial port device (e.g., "/dev/ttyAMA0")
    uint32_t baud_rate;      // Baud rate (125000 or 250000)
    uint16_t manufacturer_id;// Manufacturer ID (16-bit)
    uint16_t device_id;      // Device ID (16-bit)
    uint8_t update_rate_hz;  // Telemetry update rate in Hz (5-100)
    bool text_messages;      // Enable text messages
    bool remote_config;      // Enable remote configuration via transmitter
    JetiEXConfigCallback config_changed_callback;  // Called when parameter changes
    JetiEXConfigSaveCallback config_save_callback; // Called when save is requested
    void *user_data;         // User data passed to callbacks
} JetiEXConfig;

/* JetiEX Context */
typedef struct JetiEX JetiEX;

/**
 * @brief Create and initialize JetiEX telemetry
 * 
 * @param config Configuration structure
 * @return JetiEX context or NULL on error
 */
JetiEX *jetiex_create(const JetiEXConfig *config);

/**
 * @brief Destroy JetiEX context and close serial port
 * 
 * @param jetiex JetiEX context
 */
void jetiex_destroy(JetiEX *jetiex);

/**
 * @brief Add a sensor to the telemetry stream
 * 
 * @param jetiex JetiEX context
 * @param sensor Sensor definition
 * @return true on success, false on error
 */
bool jetiex_add_sensor(JetiEX *jetiex, const JetiEXSensor *sensor);

/**
 * @brief Update sensor value
 * 
 * @param jetiex JetiEX context
 * @param sensor_id Sensor ID (0-15)
 * @param value New value
 * @return true on success, false on error
 */
bool jetiex_update_sensor(JetiEX *jetiex, uint8_t sensor_id, int32_t value);

/**
 * @brief Send a text message to the transmitter
 * 
 * @param jetiex JetiEX context
 * @param message Text message (max 32 characters)
 * @return true on success, false on error
 */
bool jetiex_send_text(JetiEX *jetiex, const char *message);

/**
 * @brief Get sensor count
 * 
 * @param jetiex JetiEX context
 * @return Number of active sensors
 */
int jetiex_get_sensor_count(const JetiEX *jetiex);

/**
 * @brief Enable/disable a sensor
 * 
 * @param jetiex JetiEX context
 * @param sensor_id Sensor ID (0-15)
 * @param enabled Enable state
 * @return true on success, false on error
 */
bool jetiex_enable_sensor(JetiEX *jetiex, uint8_t sensor_id, bool enabled);

/**
 * @brief Start telemetry transmission thread
 * 
 * Starts background thread that periodically sends telemetry data
 * 
 * @param jetiex JetiEX context
 * @return true on success, false on error
 */
bool jetiex_start(JetiEX *jetiex);

/**
 * @brief Stop telemetry transmission thread
 * 
 * @param jetiex JetiEX context
 */
void jetiex_stop(JetiEX *jetiex);

/**
 * @brief Check if telemetry is running
 * 
 * @param jetiex JetiEX context
 * @return true if running, false otherwise
 */
bool jetiex_is_running(const JetiEX *jetiex);

/* Configuration Parameter Management */

/**
 * @brief Add a configurable parameter
 * 
 * @param jetiex JetiEX context
 * @param param Parameter definition
 * @return true on success, false on error
 */
bool jetiex_add_parameter(JetiEX *jetiex, const JetiEXParameter *param);

/**
 * @brief Remove a parameter
 * 
 * @param jetiex JetiEX context
 * @param param_id Parameter ID
 * @return true on success, false on error
 */
bool jetiex_remove_parameter(JetiEX *jetiex, uint8_t param_id);

/**
 * @brief Get parameter count
 * 
 * @param jetiex JetiEX context
 * @return Number of registered parameters
 */
int jetiex_get_parameter_count(const JetiEX *jetiex);

/**
 * @brief Get parameter by ID
 * 
 * @param jetiex JetiEX context
 * @param param_id Parameter ID
 * @return Parameter pointer or NULL if not found
 */
const JetiEXParameter *jetiex_get_parameter(const JetiEX *jetiex, uint8_t param_id);

/**
 * @brief Update parameter value (programmatically)
 * 
 * This updates the value at the pointer location. Does not trigger callback.
 * 
 * @param jetiex JetiEX context
 * @param param_id Parameter ID
 * @param value New value (type depends on parameter definition)
 * @return true on success, false on error
 */
bool jetiex_update_parameter(JetiEX *jetiex, uint8_t param_id, const void *value);

/* Helper functions for common sensor configurations */

/**
 * @brief Create an RPM sensor
 * 
 * @param id Sensor ID
 * @param label Sensor label
 * @return Configured sensor structure
 */
JetiEXSensor jetiex_sensor_rpm(uint8_t id, const char *label);

/**
 * @brief Create a voltage sensor
 * 
 * @param id Sensor ID
 * @param label Sensor label
 * @param precision Decimal places (0-2)
 * @return Configured sensor structure
 */
JetiEXSensor jetiex_sensor_voltage(uint8_t id, const char *label, uint8_t precision);

/**
 * @brief Create a current sensor
 * 
 * @param id Sensor ID
 * @param label Sensor label
 * @param precision Decimal places (0-2)
 * @return Configured sensor structure
 */
JetiEXSensor jetiex_sensor_current(uint8_t id, const char *label, uint8_t precision);

/**
 * @brief Create a temperature sensor
 * 
 * @param id Sensor ID
 * @param label Sensor label
 * @param precision Decimal places (0-1)
 * @return Configured sensor structure
 */
JetiEXSensor jetiex_sensor_temperature(uint8_t id, const char *label, uint8_t precision);

/**
 * @brief Create a percentage sensor
 * 
 * @param id Sensor ID
 * @param label Sensor label
 * @return Configured sensor structure
 */
JetiEXSensor jetiex_sensor_percentage(uint8_t id, const char *label);

/**
 * @brief Create an index/enumeration sensor (for states, modes, etc.)
 * 
 * @param id Sensor ID
 * @param label Sensor label
 * @return Configured sensor structure
 */
JetiEXSensor jetiex_sensor_index(uint8_t id, const char *label);

#endif // JETIEX_H
