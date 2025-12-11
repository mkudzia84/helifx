#include "smoke_generator.h"
#include "gpio.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

struct SmokeGenerator {
    int heater_pin;
    int fan_pin;
    bool heater_on;
    bool fan_on;
    mtx_t mutex;
};

SmokeGenerator* smoke_generator_create(int heater_pin, int fan_pin) {
    if (heater_pin < 0 || fan_pin < 0) {
        LOG_ERROR(LOG_SMOKE, "Invalid pin numbers");
        return NULL;
    }
    
    SmokeGenerator *smoke = calloc(1, sizeof(SmokeGenerator));
    if (!smoke) {
        LOG_ERROR(LOG_SMOKE, "Cannot allocate memory for smoke generator");
        return NULL;
    }
    
    smoke->heater_pin = heater_pin;
    smoke->fan_pin = fan_pin;
    smoke->heater_on = false;
    smoke->fan_on = false;
    mtx_init(&smoke->mutex, mtx_plain);
    
    // Set pins as outputs and initialize to LOW
    if (gpio_set_mode(heater_pin, GPIO_MODE_OUTPUT) < 0) {
        LOG_ERROR(LOG_SMOKE, "Failed to set heater pin %d as output", heater_pin);
        mtx_destroy(&smoke->mutex);
        free(smoke);
        return NULL;
    }
    
    if (gpio_set_mode(fan_pin, GPIO_MODE_OUTPUT) < 0) {
        LOG_ERROR(LOG_SMOKE, "Failed to set fan pin %d as output", fan_pin);
        mtx_destroy(&smoke->mutex);
        free(smoke);
        return NULL;
    }
    
    gpio_write(heater_pin, 0);
    gpio_write(fan_pin, 0);
    
    LOG_INFO(LOG_SMOKE, "Created (heater: pin %d, fan: pin %d)", heater_pin, fan_pin);
    return smoke;
}

void smoke_generator_destroy(SmokeGenerator *smoke) {
    if (!smoke) return;
    
    // Turn off both heater and fan
    gpio_write(smoke->heater_pin, 0);
    gpio_write(smoke->fan_pin, 0);
    
    mtx_destroy(&smoke->mutex);
    free(smoke);
    
    printf("[SMOKE] Smoke generator destroyed\n");
}

int smoke_generator_heater_on(SmokeGenerator *smoke) {
    if (!smoke) return -1;
    
    mtx_lock(&smoke->mutex);
    smoke->heater_on = true;
    mtx_unlock(&smoke->mutex);
    
    gpio_write(smoke->heater_pin, 1);
    printf("[SMOKE] Heater ON\n");
    
    return 0;
}

int smoke_generator_heater_off(SmokeGenerator *smoke) {
    if (!smoke) return -1;
    
    mtx_lock(&smoke->mutex);
    smoke->heater_on = false;
    mtx_unlock(&smoke->mutex);
    
    gpio_write(smoke->heater_pin, 0);
    printf("[SMOKE] Heater OFF\n");
    
    return 0;
}

int smoke_generator_fan_on(SmokeGenerator *smoke) {
    if (!smoke) return -1;
    
    mtx_lock(&smoke->mutex);
    smoke->fan_on = true;
    mtx_unlock(&smoke->mutex);
    
    gpio_write(smoke->fan_pin, 1);
    printf("[SMOKE] Fan ON\n");
    
    return 0;
}

int smoke_generator_fan_off(SmokeGenerator *smoke) {
    if (!smoke) return -1;
    
    mtx_lock(&smoke->mutex);
    smoke->fan_on = false;
    mtx_unlock(&smoke->mutex);
    
    gpio_write(smoke->fan_pin, 0);
    printf("[SMOKE] Fan OFF\n");
    
    return 0;
}

bool smoke_generator_is_heater_on(SmokeGenerator *smoke) {
    if (!smoke) return false;
    
    mtx_lock(&smoke->mutex);
    bool on = smoke->heater_on;
    mtx_unlock(&smoke->mutex);
    
    return on;
}

bool smoke_generator_is_fan_on(SmokeGenerator *smoke) {
    if (!smoke) return false;
    
    mtx_lock(&smoke->mutex);
    bool on = smoke->fan_on;
    mtx_unlock(&smoke->mutex);
    
    return on;
}
