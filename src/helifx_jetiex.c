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
        .serial_port = {0},
        .baud_rate = config->jetiex.baud_rate,
        .manufacturer_id = config->jetiex.manufacturer_id,
        .device_id = config->jetiex.device_id,
        .update_rate_hz = config->jetiex.update_rate_hz,
        .text_messages = true
    };
    strncpy(jetiex_config.serial_port, config->jetiex.serial_port, 
            sizeof(jetiex_config.serial_port) - 1);
    
    JetiEX *jetiex = jetiex_create(&jetiex_config);
    if (!jetiex) {
        LOG_ERROR(LOG_JETIEX, "Failed to create JetiEX telemetry");
        return nullptr;
    }
    
    // Add telemetry sensors
    JetiEXSensor gun_rate_sensor = jetiex_sensor_index(0, "Gun Rate");
    jetiex_add_sensor(jetiex, &gun_rate_sensor);
    
    JetiEXSensor engine_state_sensor = jetiex_sensor_index(1, "Engine State");
    jetiex_add_sensor(jetiex, &engine_state_sensor);
    
    JetiEXSensor ammo_sensor = jetiex_sensor_percentage(2, "Ammunition");
    jetiex_add_sensor(jetiex, &ammo_sensor);
    
    // Add configuration parameters if remote config is enabled
    if (config->jetiex.remote_config) {
        LOG_INFO(LOG_JETIEX, "Enabling remote configuration");
        
        // Parameter 0: Gun Rate 1 RPM
        if (config->gun.rate_count > 0) {
            JetiEXParameter param = {
                .id = 0,
                .type = JETIEX_PARAM_UINT16,
                .min.u16 = 0,
                .max.u16 = 1000,
                .value.u16 = config->gun.rates[0].rpm,
                .read_only = false,
                .persistent = true
            };
            strncpy(param.label, "Gun Rate 1 RPM", sizeof(param.label) - 1);
            param.label[sizeof(param.label) - 1] = '\0';
            param.on_change = on_parameter_change;
            jetiex_add_parameter(jetiex, &param);
        }
        
        // Parameter 1: Gun Rate 2 RPM
        if (config->gun.rate_count > 1) {
            JetiEXParameter param = {
                .id = 1,
                .type = JETIEX_PARAM_UINT16,
                .min.u16 = 0,
                .max.u16 = 1000,
                .value.u16 = config->gun.rates[1].rpm,
                .read_only = false,
                .persistent = true
            };
            strncpy(param.label, "Gun Rate 2 RPM", sizeof(param.label) - 1);
            param.label[sizeof(param.label) - 1] = '\0';
            param.on_change = on_parameter_change;
            jetiex_add_parameter(jetiex, &param);
        }
        
        // Parameter 2: Gun Rate 1 PWM
        if (config->gun.rate_count > 0) {
            JetiEXParameter param = {
                .id = 2,
                .type = JETIEX_PARAM_UINT16,
                .min.u16 = 1000,
                .max.u16 = 2000,
                .value.u16 = config->gun.rates[0].pwm_threshold_us,
                .read_only = false,
                .persistent = true
            };
            strncpy(param.label, "Gun Rate 1 PWM", sizeof(param.label) - 1);
            param.label[sizeof(param.label) - 1] = '\0';
            param.on_change = on_parameter_change;
            jetiex_add_parameter(jetiex, &param);
        }
        
        // Parameter 3: Gun Rate 2 PWM
        if (config->gun.rate_count > 1) {
            JetiEXParameter param = {
                .id = 3,
                .type = JETIEX_PARAM_UINT16,
                .min.u16 = 1000,
                .max.u16 = 2000,
                .value.u16 = config->gun.rates[1].pwm_threshold_us,
                .read_only = false,
                .persistent = true
            };
            strncpy(param.label, "Gun Rate 2 PWM", sizeof(param.label) - 1);
            param.label[sizeof(param.label) - 1] = '\0';
            param.on_change = on_parameter_change;
            jetiex_add_parameter(jetiex, &param);
        }
        
        // Parameter 4: Smoke Fan Delay
        JetiEXParameter smoke_delay_param = {
            .id = 4,
            .type = JETIEX_PARAM_UINT16,
            .min.u16 = 0,
            .max.u16 = 5000,
            .value.u16 = config->gun.smoke_fan_off_delay_ms,
            .read_only = false,
            .persistent = true
        };
        strncpy(smoke_delay_param.label, "Smoke Fan Delay", sizeof(smoke_delay_param.label) - 1);
        smoke_delay_param.label[sizeof(smoke_delay_param.label) - 1] = '\0';
        smoke_delay_param.on_change = on_parameter_change;
        jetiex_add_parameter(jetiex, &smoke_delay_param);
        
        // Parameter 5: Heater PWM Threshold
        JetiEXParameter heater_param = {
            .id = 5,
            .type = JETIEX_PARAM_UINT16,
            .min.u16 = 1000,
            .max.u16 = 2000,
            .value.u16 = config->gun.smoke_heater_pwm_threshold_us,
            .read_only = false,
            .persistent = true
        };
        strncpy(heater_param.label, "Heater PWM Thresh", sizeof(heater_param.label) - 1);
        heater_param.label[sizeof(heater_param.label) - 1] = '\0';
        heater_param.on_change = on_parameter_change;
        jetiex_add_parameter(jetiex, &heater_param);
        
        // Parameter 6: Engine PWM Threshold
        JetiEXParameter engine_param = {
            .id = 6,
            .type = JETIEX_PARAM_UINT16,
            .min.u16 = 1000,
            .max.u16 = 2000,
            .value.u16 = config->engine.threshold_us,
            .read_only = false,
            .persistent = true
        };
        strncpy(engine_param.label, "Engine PWM Thresh", sizeof(engine_param.label) - 1);
        engine_param.label[sizeof(engine_param.label) - 1] = '\0';
        engine_param.on_change = on_parameter_change;
        jetiex_add_parameter(jetiex, &engine_param);
        
        // Parameter 7: Servo Max Speed
        uint16_t servo_speed = 500;
        if (config->gun.pitch_servo.enabled) {
            servo_speed = (uint16_t)config->gun.pitch_servo.max_speed_us_per_sec;
        } else if (config->gun.yaw_servo.enabled) {
            servo_speed = (uint16_t)config->gun.yaw_servo.max_speed_us_per_sec;
        }
        JetiEXParameter servo_speed_param = {
            .id = 7,
            .type = JETIEX_PARAM_UINT16,
            .min.u16 = 0,
            .max.u16 = 2000,
            .value.u16 = servo_speed,
            .read_only = false,
            .persistent = true
        };
        strncpy(servo_speed_param.label, "Servo Max Speed", sizeof(servo_speed_param.label) - 1);
        servo_speed_param.label[sizeof(servo_speed_param.label) - 1] = '\0';
        servo_speed_param.on_change = on_parameter_change;
        jetiex_add_parameter(jetiex, &servo_speed_param);
        
        // Parameter 8: Servo Max Accel
        uint16_t servo_accel = 2000;
        if (config->gun.pitch_servo.enabled) {
            servo_accel = (uint16_t)config->gun.pitch_servo.max_accel_us_per_sec2;
        } else if (config->gun.yaw_servo.enabled) {
            servo_accel = (uint16_t)config->gun.yaw_servo.max_accel_us_per_sec2;
        }
        JetiEXParameter servo_accel_param = {
            .id = 8,
            .type = JETIEX_PARAM_UINT16,
            .min.u16 = 0,
            .max.u16 = 5000,
            .value.u16 = servo_accel,
            .read_only = false,
            .persistent = true
        };
        strncpy(servo_accel_param.label, "Servo Max Accel", sizeof(servo_accel_param.label) - 1);
        servo_accel_param.label[sizeof(servo_accel_param.label) - 1] = '\0';
        servo_accel_param.on_change = on_parameter_change;
        jetiex_add_parameter(jetiex, &servo_accel_param);
        
        // Parameter 9: Telemetry Rate Hz
        JetiEXParameter telem_rate_param = {
            .id = 9,
            .type = JETIEX_PARAM_UINT8,
            .min.u8 = 5,
            .max.u8 = 100,
            .value.u8 = config->jetiex.update_rate_hz,
            .read_only = false,
            .persistent = true
        };
        strncpy(telem_rate_param.label, "Telemetry Rate Hz", sizeof(telem_rate_param.label) - 1);
        telem_rate_param.label[sizeof(telem_rate_param.label) - 1] = '\0';
        telem_rate_param.on_change = on_parameter_change;
        jetiex_add_parameter(jetiex, &telem_rate_param);
        
        // Parameter 10: Nozzle Flash Enable
        JetiEXParameter nozzle_param = {
            .id = 10,
            .type = JETIEX_PARAM_BOOL,
            .min.b = false,
            .max.b = true,
            .value.b = config->gun.nozzle_flash_enabled,
            .read_only = false,
            .persistent = true
        };
        strncpy(nozzle_param.label, "Nozzle Flash Enable", sizeof(nozzle_param.label) - 1);
        nozzle_param.label[sizeof(nozzle_param.label) - 1] = '\0';
        nozzle_param.on_change = on_parameter_change;
        jetiex_add_parameter(jetiex, &nozzle_param);
        
        // Parameter 11: Smoke Enable
        JetiEXParameter smoke_param = {
            .id = 11,
            .type = JETIEX_PARAM_BOOL,
            .min.b = false,
            .max.b = true,
            .value.b = config->gun.smoke_enabled,
            .read_only = false,
            .persistent = true
        };
        strncpy(smoke_param.label, "Smoke Enable", sizeof(smoke_param.label) - 1);
        smoke_param.label[sizeof(smoke_param.label) - 1] = '\0';
        smoke_param.on_change = on_parameter_change;
        jetiex_add_parameter(jetiex, &smoke_param);
        
        // Set save callback
        jetiex_set_on_save_callback(jetiex, on_save_config, nullptr);
    }
    
    // Start telemetry
    if (!jetiex_start(jetiex)) {
        LOG_ERROR(LOG_JETIEX, "Failed to start JetiEX telemetry");
        jetiex_destroy(jetiex);
        return nullptr;
    }
    
    LOG_INFO(LOG_JETIEX, "JetiEX telemetry started");
    jetiex_send_text(jetiex, "HeliFX Ready");
    
    return jetiex;
}

void helifx_jetiex_update(JetiEX *jetiex, GunFX *gun, EngineFX *engine) {
    if (!jetiex) return;
    
    // Update gun rate sensor
    if (gun) {
        int gun_rate_idx = gun_fx_get_current_rate_index(gun);
        if (gun_rate_idx >= 0) {
            jetiex_update_sensor(jetiex, 0, gun_rate_idx);
        }
    }
    
    // Update engine state sensor
    if (engine) {
        EngineState engine_state_val = engine_fx_get_state(engine);
        jetiex_update_sensor(jetiex, 1, (int)engine_state_val);
    }
    
    // Update ammunition sensor (placeholder - always 100%)
    jetiex_update_sensor(jetiex, 2, 100);
}

void helifx_jetiex_cleanup(JetiEX *jetiex) {
    if (!jetiex) return;
    
    LOG_INFO(LOG_JETIEX, "Stopping JetiEX telemetry");
    jetiex_stop(jetiex);
    jetiex_destroy(jetiex);
    
    // Clear global pointers
    g_config = nullptr;
    g_config_file_path = nullptr;
    g_gun = nullptr;
    g_engine = nullptr;
}
