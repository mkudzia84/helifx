/**
 * @file helifx_jetiex.c
 * @brief JetiEX telemetry integration for HeliFX
 */

#include "helifx_jetiex.h"
#include "logging.h"
#include "servo.h"
#include <string.h>
#include <stdlib.h>

// Global state for callbacks
static HeliFXConfig *g_config = nullptr;
static char *g_config_file_path = nullptr;
static GunFX *g_gun = nullptr;
static EngineFX *g_engine = nullptr;

// JetiEX parameter change callback
static void on_parameter_change(uint8_t param_id, const void *value, void *user_data) {
    (void)user_data;
    
    if (!g_config || !g_gun) return;
    
    LOG_INFO(LOG_JETIEX, "Parameter %d changed", param_id);
    
    // Update configuration based on parameter ID
    switch (param_id) {
        case 0: // Gun Rate 1 RPM
            if (g_config->gun.rate_count > 0) {
                g_config->gun.rates[0].rpm = *(uint16_t*)value;
                LOG_INFO(LOG_JETIEX, "Gun Rate 1 RPM set to %d", *(uint16_t*)value);
            }
            break;
        case 1: // Gun Rate 2 RPM
            if (g_config->gun.rate_count > 1) {
                g_config->gun.rates[1].rpm = *(uint16_t*)value;
                LOG_INFO(LOG_JETIEX, "Gun Rate 2 RPM set to %d", *(uint16_t*)value);
            }
            break;
        case 2: // Gun Rate 1 PWM
            if (g_config->gun.rate_count > 0) {
                g_config->gun.rates[0].pwm_threshold_us = *(uint16_t*)value;
                LOG_INFO(LOG_JETIEX, "Gun Rate 1 PWM set to %d", *(uint16_t*)value);
            }
            break;
        case 3: // Gun Rate 2 PWM
            if (g_config->gun.rate_count > 1) {
                g_config->gun.rates[1].pwm_threshold_us = *(uint16_t*)value;
                LOG_INFO(LOG_JETIEX, "Gun Rate 2 PWM set to %d", *(uint16_t*)value);
            }
            break;
        case 4: // Smoke Fan Delay
            g_config->gun.smoke_fan_off_delay_ms = *(uint16_t*)value;
            gun_fx_set_smoke_fan_off_delay(g_gun, *(uint16_t*)value);
            LOG_INFO(LOG_JETIEX, "Smoke fan delay set to %d ms", *(uint16_t*)value);
            break;
        case 5: // Heater PWM Threshold
            g_config->gun.smoke_heater_pwm_threshold_us = *(uint16_t*)value;
            LOG_INFO(LOG_JETIEX, "Heater PWM threshold set to %d", *(uint16_t*)value);
            break;
        case 6: // Engine PWM Threshold
            g_config->engine.threshold_us = *(uint16_t*)value;
            LOG_INFO(LOG_JETIEX, "Engine PWM threshold set to %d", *(uint16_t*)value);
            break;
        case 7: // Servo Max Speed
            if (g_config->gun.pitch_servo.enabled) {
                g_config->gun.pitch_servo.max_speed_us_per_sec = *(uint16_t*)value;
                Servo *pitch = gun_fx_get_pitch_servo(g_gun);
                if (pitch) servo_set_max_speed(pitch, *(uint16_t*)value);
            }
            if (g_config->gun.yaw_servo.enabled) {
                g_config->gun.yaw_servo.max_speed_us_per_sec = *(uint16_t*)value;
                Servo *yaw = gun_fx_get_yaw_servo(g_gun);
                if (yaw) servo_set_max_speed(yaw, *(uint16_t*)value);
            }
            LOG_INFO(LOG_JETIEX, "Servo max speed set to %d", *(uint16_t*)value);
            break;
        case 8: // Servo Max Accel
            if (g_config->gun.pitch_servo.enabled) {
                g_config->gun.pitch_servo.max_accel_us_per_sec2 = *(uint16_t*)value;
                Servo *pitch = gun_fx_get_pitch_servo(g_gun);
                if (pitch) servo_set_max_acceleration(pitch, *(uint16_t*)value);
            }
            if (g_config->gun.yaw_servo.enabled) {
                g_config->gun.yaw_servo.max_accel_us_per_sec2 = *(uint16_t*)value;
                Servo *yaw = gun_fx_get_yaw_servo(g_gun);
                if (yaw) servo_set_max_acceleration(yaw, *(uint16_t*)value);
            }
            LOG_INFO(LOG_JETIEX, "Servo max accel set to %d", *(uint16_t*)value);
            break;
        case 9: // Telemetry Rate Hz
            g_config->jetiex.update_rate_hz = *(uint8_t*)value;
            LOG_INFO(LOG_JETIEX, "Telemetry rate set to %d Hz", *(uint8_t*)value);
            break;
        case 10: // Nozzle Flash Enable
            g_config->gun.nozzle_flash_enabled = *(bool*)value;
            LOG_INFO(LOG_JETIEX, "Nozzle flash %s", *(bool*)value ? "enabled" : "disabled");
            break;
        case 11: // Smoke Enable
            g_config->gun.smoke_enabled = *(bool*)value;
            LOG_INFO(LOG_JETIEX, "Smoke %s", *(bool*)value ? "enabled" : "disabled");
            break;
    }
    
    // Update gun rates of fire
    if (g_gun && g_config->gun.rate_count > 0) {
        RateOfFire *rates = malloc(g_config->gun.rate_count * sizeof(RateOfFire));
        if (rates) {
            for (int i = 0; i < g_config->gun.rate_count; i++) {
                rates[i].rounds_per_minute = g_config->gun.rates[i].rpm;
                rates[i].pwm_threshold_us = g_config->gun.rates[i].pwm_threshold_us;
                rates[i].sound = nullptr; // Sound already loaded
            }
            gun_fx_set_rates_of_fire(g_gun, rates, g_config->gun.rate_count);
            free(rates);
        }
    }
}

// JetiEX save callback
static void on_save_config(void *user_data) {
    (void)user_data;
    
    if (!g_config_file_path || !g_config) {
        LOG_ERROR(LOG_JETIEX, "Cannot save: config path or data missing");
        return;
    }
    
    LOG_INFO(LOG_JETIEX, "Saving configuration to %s", g_config_file_path);
    
    if (config_save(g_config_file_path, g_config) == 0) {
        LOG_INFO(LOG_JETIEX, "Configuration saved successfully");
    } else {
        LOG_ERROR(LOG_JETIEX, "Failed to save configuration");
    }
}

JetiEX* helifx_jetiex_init(HeliFXConfig *config, const char *config_file_path,
                           GunFX *gun, EngineFX *engine) {
    if (!config || !config->jetiex.enabled) {
        return nullptr;
    }
    
    // Store global pointers for callbacks
    g_config = config;
    g_config_file_path = (char*)config_file_path;
    g_gun = gun;
    g_engine = engine;
    
    LOG_INFO(LOG_JETIEX, "Initializing JetiEX telemetry...");
    
    // Create JetiEX configuration
    JetiEXConfig jetiex_config = {
        .serial_port = config->jetiex.serial_port,
        .baud_rate = config->jetiex.baud_rate,
        .manufacturer_id = config->jetiex.manufacturer_id,
        .device_id = config->jetiex.device_id,
        .update_rate_hz = config->jetiex.update_rate_hz,
        .text_messages = true,
        .remote_config = config->jetiex.remote_config,
        .config_changed_callback = on_parameter_change,
        .config_save_callback = on_save_config,
        .user_data = nullptr
    };
    
    JetiEX *jetiex = jetiex_create(&jetiex_config);
    if (!jetiex) {
        LOG_ERROR(LOG_JETIEX, "Failed to create JetiEX telemetry");
        return nullptr;
    }
    
    LOG_INFO(LOG_JETIEX, "JetiEX initialized successfully");
    return jetiex;
}

void helifx_jetiex_update(JetiEX *jetiex, GunFX *gun, EngineFX *engine) {
    if (!jetiex) return;
    
    // Update gun rate sensor (sensor 0)
    if (gun) {
        int gun_rpm = gun_fx_get_current_rpm(gun);
        jetiex_update_sensor(jetiex, 0, gun_rpm);
    }
    
    // Update engine state sensor (sensor 1)
    if (engine) {
        EngineState state = engine_fx_get_state(engine);
        jetiex_update_sensor(jetiex, 1, (int32_t)state);
    }
}

void helifx_jetiex_cleanup(JetiEX *jetiex) {
    if (!jetiex) return;
    
    LOG_INFO(LOG_JETIEX, "Stopping JetiEX telemetry");
    jetiex_destroy(jetiex);
    
    // Clear global pointers
    g_config = nullptr;
    g_config_file_path = nullptr;
    g_gun = nullptr;
    g_engine = nullptr;
}
