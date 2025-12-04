#include "servo.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/**
 * @brief Servo Demo
 * 
 * Demonstrates servo controller with speed and acceleration limiting
 * using asynchronous processing thread
 * 
 * Usage: servo_demo [options]
 * Options:
 *   --input-min=N          Input minimum microseconds (default: 1000)
 *   --input-max=N          Input maximum microseconds (default: 2000)
 *   --output-min=N         Output minimum microseconds (default: 800)
 *   --output-max=N         Output maximum microseconds (default: 2200)
 *   --max-speed=N          Maximum speed us/sec (default: 500)
 *   --max-accel=N          Maximum acceleration us/sec² (default: 2000)
 *   --update-rate=N        Update rate Hz (default: 50)
 *   --help                 Show this help message
 */

void print_usage(const char *prog) {
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  --input-min=N          Input minimum microseconds (default: 1000)\n");
    printf("  --input-max=N          Input maximum microseconds (default: 2000)\n");
    printf("  --output-min=N         Output minimum microseconds (default: 800)\n");
    printf("  --output-max=N         Output maximum microseconds (default: 2200)\n");
    printf("  --max-speed=N          Maximum speed us/sec (default: 500)\n");
    printf("  --max-accel=N          Maximum acceleration us/sec² (default: 2000)\n");
    printf("  --update-rate=N        Update rate Hz (default: 50)\n");
    printf("  --help                 Show this help message\n");
}

int main(int argc, char *argv[]) {
    // Default configuration
    ServoConfig config = {
        .input_min_us = 1000,
        .input_max_us = 2000,
        .output_min_us = 800,
        .output_max_us = 2200,
        .max_speed_us_per_sec = 500.0f,
        .max_accel_us_per_sec2 = 2000.0f,
        .update_rate_hz = 50
    };
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--input-min=", 12) == 0) {
            config.input_min_us = atoi(argv[i] + 12);
        } else if (strncmp(argv[i], "--input-max=", 12) == 0) {
            config.input_max_us = atoi(argv[i] + 12);
        } else if (strncmp(argv[i], "--output-min=", 13) == 0) {
            config.output_min_us = atoi(argv[i] + 13);
        } else if (strncmp(argv[i], "--output-max=", 13) == 0) {
            config.output_max_us = atoi(argv[i] + 13);
        } else if (strncmp(argv[i], "--max-speed=", 12) == 0) {
            config.max_speed_us_per_sec = atof(argv[i] + 12);
        } else if (strncmp(argv[i], "--max-accel=", 12) == 0) {
            config.max_accel_us_per_sec2 = atof(argv[i] + 12);
        } else if (strncmp(argv[i], "--update-rate=", 14) == 0) {
            config.update_rate_hz = atoi(argv[i] + 14);
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    printf("=== Servo Controller Demo ===\n\n");
    
    Servo *servo = servo_create(&config);
    if (!servo) {
        fprintf(stderr, "Failed to create servo\n");
        return 1;
    }
    
    printf("Servo Configuration:\n");
    printf("  Input:  %d - %d us\n", config.input_min_us, config.input_max_us);
    printf("  Output: %d - %d us\n", config.output_min_us, config.output_max_us);
    printf("  Max Speed: %.0f us/sec\n", config.max_speed_us_per_sec);
    printf("  Max Accel: %.0f us/sec²\n", config.max_accel_us_per_sec2);
    printf("  Update Rate: %d Hz\n\n", config.update_rate_hz);
    
    // Test sequence: center -> max -> min -> center
    int test_inputs[] = {1500, 2000, 1000, 1500};
    const char *test_names[] = {"Center", "Maximum", "Minimum", "Center"};
    
    for (int test = 0; test < 4; test++) {
        printf("Moving to %s (%d us input)...\n", test_names[test], test_inputs[test]);
        
        // Set new input value (servo thread handles the rest)
        servo_set_input(servo, test_inputs[test]);
        
        // Monitor progress until settled (5 consecutive readings at target)
        int settled_count = 0;
        while (settled_count < 5) {
            int output = servo_get_output(servo);
            int target = servo_get_target(servo);
            float velocity = servo_get_velocity(servo);
            
            printf("  Output: %4d us  Target: %4d us  Velocity: %6.0f us/s  Error: %4d us\n",
                   output, target, velocity, target - output);
            
            // Check if settled
            if (abs(output - target) < 1) {
                settled_count++;
            } else {
                settled_count = 0;
            }
            
            usleep(100000); // Check every 100ms
        }
        
        printf("  ✓ Position reached\n\n");
        sleep(1); // Pause between movements
    }
    
    printf("Testing instant position change (no limits)...\n");
    config.max_speed_us_per_sec = 0.0f;
    config.max_accel_us_per_sec2 = 0.0f;
    servo_set_config(servo, &config);
    
    servo_set_input(servo, 2000);
    usleep(100000); // Wait for thread to process
    printf("  Input: 2000 us -> Output: %d us (instant)\n\n", servo_get_output(servo));
    
    printf("Testing manual reset...\n");
    servo_reset(servo, 1500);
    printf("  Reset to: %d us\n\n", servo_get_output(servo));
    
    servo_destroy(servo);
    printf("=== Demo Complete ===\n");
    
    return 0;
}
