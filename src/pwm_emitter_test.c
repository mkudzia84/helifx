#include "gpio.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

static volatile sig_atomic_t running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <gpio_pin> [--freq HZ] [--width US] [--verbose]\n", argv[0]);
        fprintf(stderr, "  Examples:\n");
        fprintf(stderr, "    %s 8 --verbose               (50Hz sweep 1000-2000us)\n", argv[0]);
        fprintf(stderr, "    %s 8 --freq 100              (100Hz sweep)\n", argv[0]);
        fprintf(stderr, "    %s 8 --width 1500 --verbose  (hold 1500us at 50Hz)\n", argv[0]);
        return 1;
    }

    int pin = atoi(argv[1]);
    int freq_hz = 0;
    int width_us = 0; // if >0, hold fixed width
    int verbose = 0;
    // Parse simple flags: --freq N, --width N, --verbose/-v
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "--freq") == 0 && i + 1 < argc) {
            freq_hz = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            width_us = atoi(argv[++i]);
        } else {
            fprintf(stderr, "Unknown or incomplete argument: %s\n", argv[i]);
            return 1;
        }
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    if (logging_init(NULL, 0, 0) != 0) {
        fprintf(stderr, "Failed to initialize logging\n");
        return 1;
    }

    if (freq_hz > 0 && width_us > 0) {
        LOG_INFO(LOG_SYSTEM, "PWM emitter test on GPIO %d at %d Hz, width %d us", pin, freq_hz, width_us);
    } else if (freq_hz > 0) {
        LOG_INFO(LOG_SYSTEM, "PWM emitter test on GPIO %d at %d Hz (sweep mode)", pin, freq_hz);
    } else if (width_us > 0) {
        LOG_INFO(LOG_SYSTEM, "PWM emitter test on GPIO %d (50 Hz default), width %d us", pin, width_us);
    } else {
        LOG_INFO(LOG_SYSTEM, "PWM emitter test on GPIO %d (50 Hz default, sweep mode)", pin);
    }

    int exit_code = 0;
    PWMEmitter *emitter = NULL;

    if (gpio_init() < 0) {
        LOG_ERROR(LOG_SYSTEM, "Failed to initialize GPIO");
        exit_code = 1;
        goto cleanup;
    }

    emitter = pwm_emitter_create(pin, "test");
    if (!emitter) {
        LOG_ERROR(LOG_SYSTEM, "Failed to create PWM emitter on pin %d", pin);
        exit_code = 1;
        goto cleanup;
    }
    if (freq_hz > 0) {
        pwm_emitter_set_frequency(emitter, freq_hz);
    }
    if (width_us > 0) {
        // Fixed-width mode
        if (pwm_emitter_set_value(emitter, width_us) != 0) {
            LOG_ERROR(LOG_SYSTEM, "Failed to set PWM width to %d us", width_us);
            exit_code = 1;
            goto cleanup;
        }
        int tick = 0;
        while (running) {
            if (verbose && (tick % 20 == 0)) {
                LOG_INFO(LOG_SYSTEM, "PWM width: %d us (freq: %d Hz)", pwm_emitter_get_value(emitter), pwm_emitter_get_frequency(emitter));
            }
            tick++;
            usleep(25 * 1000); // keep modest console rate
        }
    } else {
        // Sweep mode: Cycle from 1000us to 2000us and back, ~5s full cycle
        const int min_us = 1000;
        const int max_us = 2000;
        const int step_ms = 25;          // 25ms step -> ~5s full sweep (2.5s each direction)
        int direction = 1;
        int value = min_us;
        int step_count = 0;

        while (running) {
            if (pwm_emitter_set_value(emitter, value) != 0) {
                LOG_ERROR(LOG_SYSTEM, "Failed to set PWM value to %d us", value);
                exit_code = 1;
                break;
            }

            if (verbose && (step_count % 20 == 0)) {
                LOG_INFO(LOG_SYSTEM, "PWM value: %d us", value);
            }
            step_count++;

            // Update value
            value += direction * 10; // 10us per step
            if (value >= max_us) {
                value = max_us;
                direction = -1;
            } else if (value <= min_us) {
                value = min_us;
                direction = 1;
            }

            usleep(step_ms * 1000);
        }
    }

    LOG_INFO(LOG_SYSTEM, "Stopping PWM emitter test");

cleanup:
    if (emitter) {
        pwm_emitter_destroy(emitter);
    }
    gpio_cleanup();
    logging_shutdown();
    return exit_code;
}
