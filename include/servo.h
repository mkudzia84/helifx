#ifndef SERVO_H
#define SERVO_H

#include <stdbool.h>

/**
 * @file servo.h
 * @brief Servo controller with configurable mapping, speed, and acceleration
 * 
 * This module maps an input PWM signal to an output PWM signal with:
 * - Configurable input/output range mapping
 * - Speed limiting (maximum rate of change)
 * - Acceleration limiting (smooth ramping)
 * - Asynchronous processing thread
 */

typedef struct Servo Servo;

/**
 * @brief Configuration for servo behavior
 */
typedef struct {
    int input_min_us;       // Minimum input PWM pulse width (microseconds)
    int input_max_us;       // Maximum input PWM pulse width (microseconds)
    int output_min_us;      // Minimum output PWM pulse width (microseconds)
    int output_max_us;      // Maximum output PWM pulse width (microseconds)
    float max_speed_us_per_sec;    // Maximum speed in us/second (0 = unlimited)
    float max_accel_us_per_sec2;   // Maximum acceleration in us/second² (0 = unlimited)
    int update_rate_hz;     // Update rate for servo thread (default: 50Hz)
} ServoConfig;

/**
 * @brief Create a new servo controller
 * 
 * Creates a servo with an asynchronous processing thread that handles
 * smooth motion control based on speed and acceleration limits.
 * 
 * @param config Servo configuration
 * @return Servo instance or NULL on failure
 */
Servo* servo_create(const ServoConfig *config);

/**
 * @brief Destroy servo controller and free resources
 * 
 * Stops the processing thread and releases all resources.
 * 
 * @param servo Servo instance to destroy
 */
void servo_destroy(Servo *servo);

/**
 * @brief Set input value
 * 
 * Thread-safe method to update the input PWM value.
 * The servo thread will automatically process this and update the output.
 * 
 * @param servo Servo instance
 * @param input_us Input PWM pulse width in microseconds
 */
void servo_set_input(Servo *servo, int input_us);

/**
 * @brief Get current output value
 * 
 * Thread-safe method to read the current output PWM value.
 * 
 * @param servo Servo instance
 * @return Current output PWM pulse width in microseconds
 */
int servo_get_output(const Servo *servo);

/**
 * @brief Get current target value (mapped from input)
 * 
 * @param servo Servo instance
 * @return Target output PWM pulse width in microseconds
 */
int servo_get_target(const Servo *servo);

/**
 * @brief Get current velocity
 * 
 * @param servo Servo instance
 * @return Current velocity in us/second
 */
float servo_get_velocity(const Servo *servo);

/**
 * @brief Reset servo to specific position instantly
 * 
 * @param servo Servo instance
 * @param position_us Position in microseconds
 */
void servo_reset(Servo *servo, int position_us);

/**
 * @brief Update servo configuration
 * 
 * Thread-safe method to update configuration at runtime.
 * 
 * @param servo Servo instance
 * @param config New configuration
 * @return 0 on success, -1 on failure
 */
int servo_set_config(Servo *servo, const ServoConfig *config);

/**
 * @brief Get current servo configuration
 * 
 * @param servo Servo instance
 * @param config Output configuration structure
 * @return 0 on success, -1 on failure
 */
int servo_get_config(const Servo *servo, ServoConfig *config);

#endif // SERVO_H
