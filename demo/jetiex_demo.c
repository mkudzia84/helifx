/**
 * @file jetiex_demo.c
 * @brief JetiEX Telemetry Demo
 * 
 * Demonstrates JetiEX telemetry with simulated sensor data:
 * - Gun Rate Index (0-2, which firing rate is active)
 * - Engine State (0-3, engine state machine: STOPPED/STARTING/RUNNING/STOPPING)
 * - Battery voltage
 * - Temperature
 * - Ammunition percentage
 * 
 * Usage: jetiex_demo [options]
 * Options:
 *   --serial=PORT          Serial port (default: /dev/ttyAMA0)
 *   --baud=RATE           Baud rate (default: 115200)
 *   --rate=HZ             Update rate in Hz (default: 10)
 *   --manufacturer=ID     Manufacturer ID hex (default: 0xA409)
 *   --device=ID           Device ID hex (default: 0x0001)
 *   --help                Show this help message
 * 
 * Examples:
 *   jetiex_demo
 *   jetiex_demo --serial=/dev/ttyUSB0 --baud=9600
 *   jetiex_demo --rate=20 --manufacturer=0x1234
 */

#include "jetiex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

static volatile bool running = true;

void signal_handler(int signum) {
    (void)signum;
    printf("\n[DEMO] Shutting down...\n");
    running = false;
}

void print_usage(const char *prog) {
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  --serial=PORT          Serial port (default: /dev/ttyAMA0)\n");
    printf("  --baud=RATE           Baud rate (default: 115200)\n");
    printf("  --rate=HZ             Update rate in Hz (default: 10)\n");
    printf("  --manufacturer=ID     Manufacturer ID hex (default: 0xA409)\n");
    printf("  --device=ID           Device ID hex (default: 0x0001)\n");
    printf("  --help                Show this help message\n");
    printf("\n");
    printf("Simulated Sensors:\n");
    printf("  - Gun Rate Index (0-2)\n");
    printf("  - Engine State (0=STOPPED, 1=STARTING, 2=RUNNING, 3=STOPPING)\n");
    printf("  - Ammunition (0-100%%)\n");
}

int main(int argc, char *argv[]) {
    // Default configuration
    JetiEXConfig config = {
        .serial_port = "/dev/ttyAMA0",
        .baud_rate = 115200,
        .manufacturer_id = 0xA409,  // Example manufacturer ID
        .device_id = 0x0001,         // Device ID
        .update_rate_hz = 10,
        .text_messages = true
    };
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--serial=", 9) == 0) {
            config.serial_port = argv[i] + 9;
        } else if (strncmp(argv[i], "--baud=", 7) == 0) {
            config.baud_rate = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--rate=", 7) == 0) {
            config.update_rate_hz = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--manufacturer=", 15) == 0) {
            config.manufacturer_id = strtol(argv[i] + 15, nullptr, 16);
        } else if (strncmp(argv[i], "--device=", 9) == 0) {
            config.device_id = strtol(argv[i] + 9, nullptr, 16);
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    printf("=== JetiEX Telemetry Demo ===\n\n");
    
    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create JetiEX telemetry
    JetiEX *jetiex = jetiex_create(&config);
    if (!jetiex) {
        fprintf(stderr, "Failed to create JetiEX telemetry\n");
        return 1;
    }
    
    printf("Configuration:\n");
    printf("  Serial Port:     %s\n", config.serial_port);
    printf("  Baud Rate:       %u\n", config.baud_rate);
    printf("  Update Rate:     %u Hz\n", config.update_rate_hz);
    printf("  Manufacturer ID: 0x%04X\n", config.manufacturer_id);
    printf("  Device ID:       0x%04X\n", config.device_id);
    printf("\n");
    
    // Add sensors
    JetiEXSensor gun_rate_sensor = jetiex_sensor_index(0, "Gun Rate");
    jetiex_add_sensor(jetiex, &gun_rate_sensor);
    
    JetiEXSensor engine_state_sensor = jetiex_sensor_index(1, "Engine State");
    jetiex_add_sensor(jetiex, &engine_state_sensor);
    
    JetiEXSensor ammo = jetiex_sensor_percentage(2, "Ammunition");
    jetiex_add_sensor(jetiex, &ammo);
    
    printf("Sensors added: %d\n\n", jetiex_get_sensor_count(jetiex));
    
    // Start telemetry
    if (!jetiex_start(jetiex)) {
        fprintf(stderr, "Failed to start telemetry\n");
        jetiex_destroy(jetiex);
        return 1;
    }
    
    printf("Telemetry started. Press Ctrl+C to stop.\n");
    printf("────────────────────────────────────────────────────────────────\n\n");
    
    // Send initial text message
    jetiex_send_text(jetiex, "HeliFX Ready");
    
    // Simulation variables
    int gun_rate_index = 0;  // 0=low, 1=medium, 2=high rate of fire
    int engine_state = 0;     // 0=STOPPED, 1=STARTING, 2=RUNNING, 3=STOPPING
    int ammo_value = 100;
    bool firing = false;
    int cycle_count = 0;
    
    // State names for display
    const char *engine_states[] = {"STOPPED", "STARTING", "RUNNING", "STOPPING"};
    const char *gun_rates[] = {"Low", "Medium", "High"};
    
    srand(time(nullptr));
    
    // Main loop - update sensor values
    while (running) {
        cycle_count++;
        
        // Simulate gun rate changes (cycle through rates every 10 seconds)
        if (cycle_count % 100 == 0) {
            gun_rate_index = (gun_rate_index + 1) % 3;  // Cycle 0->1->2->0
            char msg[32];
            snprintf(msg, sizeof(msg), "Gun: %s Rate", gun_rates[gun_rate_index]);
            jetiex_send_text(jetiex, msg);
        }
        
        // Simulate gun firing (toggle every 5 seconds)
        if (cycle_count % 50 == 0) {
            firing = !firing;
            if (firing) {
                ammo_value -= 5;
                if (ammo_value < 0) ammo_value = 0;
                jetiex_send_text(jetiex, "Gun Firing!");
            } else {
                jetiex_send_text(jetiex, "Gun Idle");
            }
        }
        
        // Simulate engine state transitions (cycle every 8 seconds)
        if (cycle_count % 80 == 0) {
            engine_state = (engine_state + 1) % 4;  // Cycle 0->1->2->3->0
            char msg[32];
            snprintf(msg, sizeof(msg), "Engine: %s", engine_states[engine_state]);
            jetiex_send_text(jetiex, msg);
        }
        
        // Reload ammunition periodically
        if (cycle_count % 200 == 0 && ammo_value < 100) {
            ammo_value = 100;
            jetiex_send_text(jetiex, "Ammo Reloaded");
        }
        
        // Update sensor values
        jetiex_update_sensor(jetiex, 0, gun_rate_index);
        jetiex_update_sensor(jetiex, 1, engine_state);
        jetiex_update_sensor(jetiex, 2, ammo_value);
        
        // Print status
        printf("\r[%d] Gun: %s (%d) | Engine: %s (%d) | Ammo: %3d%%  ",
               cycle_count,
               gun_rates[gun_rate_index],
               gun_rate_index,
               engine_states[engine_state],
               engine_state,
               ammo_value);
        fflush(stdout);
        
        usleep(100000);  // 100ms update
    }
    
    printf("\n\n");
    
    // Cleanup
    jetiex_stop(jetiex);
    jetiex_destroy(jetiex);
    
    printf("Demo completed.\n");
    
    return 0;
}
