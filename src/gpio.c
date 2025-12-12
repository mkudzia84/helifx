#include "gpio.h"
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <threads.h>
#include <poll.h>
#include "logging.h"

static bool initialized = false;
static struct gpiod_chip *chip = NULL;

// Single monitoring thread for all PWM pins
static thrd_t pwm_monitoring_thread;
static bool pwm_thread_running = false;
static mtx_t pwm_monitors_mutex;

#define MAX_PWM_MONITORS 8
static PWMMonitor *active_monitors[MAX_PWM_MONITORS] = {0};
static int active_monitor_count = 0;

// WM8960 Audio HAT reserved pins - DO NOT USE
#define WM8960_I2C_SDA    2   // I2C Data
#define WM8960_I2C_SCL    3   // I2C Clock
#define WM8960_I2S_BCK    18  // I2S Bit Clock
#define WM8960_I2S_LRCK   19  // I2S Left/Right Clock
#define WM8960_I2S_DIN    20  // I2S Data In (ADC)
#define WM8960_I2S_DOUT   21  // I2S Data Out (DAC)

// Check if a pin is reserved by WM8960 Audio HAT
static bool is_audio_hat_pin(int pin) {
    return (pin == WM8960_I2C_SDA || pin == WM8960_I2C_SCL ||
            pin == WM8960_I2S_BCK || pin == WM8960_I2S_LRCK ||
            pin == WM8960_I2S_DIN || pin == WM8960_I2S_DOUT);
}

// Track GPIO line requests we've made (libgpiod v2.x uses requests)
#define MAX_LINES 32
static struct gpiod_line_request *line_requests[MAX_LINES] = {0};

int gpio_init(void) {
    if (initialized) {
        LOG_WARN(LOG_GPIO, "GPIO already initialized");
        return 0;
    }
    
    // Open GPIO chip (gpiochip0 on Raspberry Pi)
    chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        LOG_ERROR(LOG_GPIO, "Failed to open GPIO chip: %s", strerror(errno));
        LOG_ERROR(LOG_GPIO, "Make sure you have permission to access /dev/gpiochip0");
        return -1;
    }
    
    // Initialize PWM monitoring mutex
    mtx_init(&pwm_monitors_mutex, mtx_plain);
    
    initialized = true;
    LOG_INFO(LOG_GPIO, "GPIO subsystem initialized using libgpiod v2.x");
    LOG_INFO(LOG_GPIO, "WM8960 Audio HAT pins (2,3,18-21) will not be used");
    return 0;
}

void gpio_cleanup(void) {
    if (!initialized) return;
    
    // Stop PWM monitoring thread if running
    if (pwm_thread_running) {
        pwm_thread_running = false;
        thrd_join(pwm_monitoring_thread, NULL);
    }
    
    // Release all GPIO line requests
    for (int i = 0; i < MAX_LINES; i++) {
        if (line_requests[i]) {
            gpiod_line_request_release(line_requests[i]);
            line_requests[i] = NULL;
        }
    }
    
    // Destroy PWM monitoring mutex
    mtx_destroy(&pwm_monitors_mutex);
    
    // Close GPIO chip
    if (chip) {
        gpiod_chip_close(chip);
        chip = NULL;
    }
    
    initialized = false;
    LOG_INFO(LOG_GPIO, "GPIO subsystem cleaned up");
}

int gpio_set_mode(int pin, GPIOMode mode) {
    if (!initialized || !chip) {
        LOG_ERROR(LOG_GPIO, "GPIO not initialized");
        return -1;
    }
    
    // Prevent using WM8960 Audio HAT pins
    if (is_audio_hat_pin(pin)) {
        LOG_ERROR(LOG_GPIO, "Cannot use GPIO %d - reserved for WM8960 Audio HAT!", pin);
        return -1;
    }
    
    // Release any existing request for this pin
    if (line_requests[pin]) {
        gpiod_line_request_release(line_requests[pin]);
        line_requests[pin] = NULL;
    }
    
    // Get the GPIO line
    struct gpiod_line *line = gpiod_chip_get_line(chip, pin);
    if (!line) {
        LOG_ERROR(LOG_GPIO, "Failed to get GPIO line %d", pin);
        return -1;
    }
    
    // Create request config based on mode
    struct gpiod_line_request_config config = {
        .consumer = "helifx",
        .request_type = (mode == GPIO_MODE_INPUT) ? 
                        GPIOD_LINE_REQUEST_DIRECTION_INPUT : 
                        GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
        .flags = 0
    };
    
    // Request the line
    struct gpiod_line_request *request = gpiod_line_request_new(chip, line, &config);
    if (!request) {
        LOG_ERROR(LOG_GPIO, "Failed to request GPIO line %d: %s", pin, strerror(errno));
        return -1;
    }
    
    // Store the request
    line_requests[pin] = request;
    
    LOG_INFO(LOG_GPIO, "GPIO %d configured as %s", pin, 
            mode == GPIO_MODE_INPUT ? "INPUT" : "OUTPUT");
    return 0;
}

int gpio_set_pull(int pin, GPIOPull pull) {
    if (!initialized) {
        LOG_ERROR(LOG_GPIO, "GPIO not initialized");
        return -1;
    }
    
    if (is_audio_hat_pin(pin)) {
        LOG_ERROR(LOG_GPIO, "Cannot use GPIO %d - reserved for WM8960 Audio HAT!", pin);
        return -1;
    }
    
    // Note: libgpiod v2.x doesn't support pull-up/down configuration directly
    // This is configured in device tree or requires bias flags on request
    // For now, we'll just log a warning if not GPIO_PULL_OFF
    if (pull != GPIO_PULL_OFF) {
        LOG_WARN(LOG_GPIO, "Pull-up/down configuration not supported in libgpiod v2.x");
        LOG_WARN(LOG_GPIO, "Configure pull resistors in device tree if needed");
    }
    
    return 0;
}

int gpio_set_pull(int pin, GPIOPull pull) {
    if (!initialized) {
        LOG_ERROR(LOG_GPIO, "GPIO not initialized");
        return -1;
    }
    
    if (is_audio_hat_pin(pin)) {
        LOG_ERROR(LOG_GPIO, "Cannot use GPIO %d - reserved for WM8960 Audio HAT!", pin);
        return -1;
    }
    
    // Note: libgpiod v1.x doesn't support pull-up/down configuration directly
    // This is configured in device tree or requires libgpiod v2.x with bias flags
    // For now, we'll just log a warning if not GPIO_PULL_OFF
    if (pull != GPIO_PULL_OFF) {
        LOG_WARN(LOG_GPIO, "Pull-up/down configuration not supported in libgpiod v1.x");
        LOG_WARN(LOG_GPIO, "Configure pull resistors in device tree if needed");
    }
    
    return 0;
}

int gpio_write(int pin, bool value) {
    if (!initialized) {
        LOG_ERROR(LOG_GPIO, "GPIO not initialized");
        return -1;
    }
    
    if (is_audio_hat_pin(pin)) {
        LOG_ERROR(LOG_GPIO, "Cannot use GPIO %d - reserved for WM8960 Audio HAT!", pin);
        return -1;
    }
    
    struct gpiod_line_request *request = line_requests[pin];
    if (!request) {
        LOG_ERROR(LOG_GPIO, "GPIO %d not configured (call gpio_set_mode first)", pin);
        return -1;
    }
    
    int result = gpiod_line_request_set_value(request, pin, value ? 1 : 0);
    if (result < 0) {
        LOG_ERROR(LOG_GPIO, "Failed to write GPIO %d: %s", pin, strerror(errno));
        return -1;
    }
    
    return 0;
}

bool gpio_read(int pin) {
    if (!initialized) {
        LOG_ERROR(LOG_GPIO, "GPIO not initialized");
        return false;
    }
    
    if (is_audio_hat_pin(pin)) {
        LOG_ERROR(LOG_GPIO, "Cannot use GPIO %d - reserved for WM8960 Audio HAT!", pin);
        return false;
    }
    
    struct gpiod_line_request *request = line_requests[pin];
    if (!request) {
        LOG_ERROR(LOG_GPIO, "GPIO %d not configured (call gpio_set_mode first)", pin);
        return false;
    }
    
    int result = gpiod_line_request_get_value(request, pin);
    if (result < 0) {
        LOG_ERROR(LOG_GPIO, "Failed to read GPIO %d: %s", pin, strerror(errno));
        return false;
    }
    
    return result != 0;
}

// ============================================================================
// ASYNC PWM MONITOR IMPLEMENTATION (using libgpiod edge detection)
// ============================================================================

struct PWMMonitor {
    int pin;
    char *feature_name;
    struct gpiod_line_request *line_request;
    mtx_t mutex;
    
    bool active;
    bool has_new_reading;
    bool first_signal_received;
    
    PWMReading current_reading;
    
    PWMCallback callback;
    void *user_data;
    
    // Track rising edge for pulse width calculation
    struct timespec rise_time;
    bool waiting_for_fall;

    // Averaging window and ring buffer for recent readings
    int avg_window_ms;
    #define PWM_AVG_MAX_SAMPLES 128
    struct {
        int duration_us;
        struct timespec ts;
    } samples[PWM_AVG_MAX_SAMPLES];
    int sample_head;
};

// Get microseconds from timespec
static int64_t timespec_to_us(const struct timespec *ts) {
    return (int64_t)ts->tv_sec * 1000000 + ts->tv_nsec / 1000;
}

// Process edge event for a specific monitor
static void process_pwm_event(PWMMonitor *monitor, struct gpiod_edge_event *event) {
    if (gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_RISING_EDGE) {
        // Rising edge - start of pulse
        uint64_t ns = gpiod_edge_event_get_timestamp_ns(event);
        monitor->rise_time.tv_sec = ns / 1000000000;
        monitor->rise_time.tv_nsec = ns % 1000000000;
        monitor->waiting_for_fall = true;
    } else if (gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_FALLING_EDGE && monitor->waiting_for_fall) {
        // Falling edge - end of pulse, calculate pulse width
        uint64_t ns = gpiod_edge_event_get_timestamp_ns(event);
        struct timespec fall_time = {
            .tv_sec = ns / 1000000000,
            .tv_nsec = ns % 1000000000
        };
        
        int64_t rise_us = timespec_to_us(&monitor->rise_time);
        int64_t fall_us = timespec_to_us(&fall_time);
        int pulse_width = (int)(fall_us - rise_us);
        
        // Sanity check: typical RC PWM is 1000-2000µs, allow 500-3000µs
        if (pulse_width >= 500 && pulse_width <= 3000) {
            if (!monitor->first_signal_received) {
                LOG_INFO(LOG_GPIO, "First PWM signal received on [%s] pin %d: %d µs",
                         monitor->feature_name ?: "Unknown", monitor->pin, pulse_width);
                monitor->first_signal_received = true;
            }
            
            PWMReading reading;
            reading.pin = monitor->pin;
            reading.duration_us = pulse_width;
            
            mtx_lock(&monitor->mutex);
            
            // Update current reading
            monitor->current_reading = reading;
            monitor->has_new_reading = true;
            
            // Append to averaging buffer
            clock_gettime(CLOCK_MONOTONIC, &monitor->samples[monitor->sample_head].ts);
            monitor->samples[monitor->sample_head].duration_us = pulse_width;
            monitor->sample_head = (monitor->sample_head + 1) % PWM_AVG_MAX_SAMPLES;
            
            mtx_unlock(&monitor->mutex);
            
            // Call user callback if set
            if (monitor->callback) {
                monitor->callback(reading, monitor->user_data);
            }
        }
        monitor->waiting_for_fall = false;
    }
}

// Single PWM monitoring thread - polls all active monitors using poll()
static int pwm_monitoring_thread_func(void *arg) {
    (void)arg;  // Unused
    
    struct pollfd fds[MAX_PWM_MONITORS];
    
    LOG_INFO(LOG_GPIO, "PWM monitoring thread started");
    
    while (pwm_thread_running) {
        // Build pollfd array from active monitors
        int nfds = 0;
        PWMMonitor *monitor_map[MAX_PWM_MONITORS] = {0};  // Map fd index to monitor
        
        mtx_lock(&pwm_monitors_mutex);
        for (int i = 0; i < MAX_PWM_MONITORS; i++) {
            if (active_monitors[i] && active_monitors[i]->active) {
                int fd = gpiod_line_request_get_fd(active_monitors[i]->line_request);
                if (fd >= 0) {
                    fds[nfds].fd = fd;
                    fds[nfds].events = POLLIN;
                    fds[nfds].revents = 0;
                    monitor_map[nfds] = active_monitors[i];
                    nfds++;
                }
            }
        }
        mtx_unlock(&pwm_monitors_mutex);
        
        if (nfds == 0) {
            // No active monitors, sleep and retry
            usleep(100000);  // 100ms
            continue;
        }
        
        // Poll with timeout
        int ret = poll(fds, nfds, 1000);  // 1 second timeout
        if (ret < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR(LOG_GPIO, "poll() error: %s", strerror(errno));
            break;
        }
        
        if (ret == 0) continue;  // Timeout
        
        // Check which monitors have events
        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                PWMMonitor *monitor = monitor_map[i];
                if (!monitor) continue;
                
                // Read all pending events for this monitor
                struct gpiod_edge_event *event;
                while ((event = gpiod_line_request_read_edge_event(monitor->line_request)) != NULL) {
                    process_pwm_event(monitor, event);
                    gpiod_edge_event_free(event);
                }
            }
        }
    }
    
    LOG_INFO(LOG_GPIO, "PWM monitoring thread stopped");
    return 0;
}

PWMMonitor* pwm_monitor_create(int pin, PWMCallback callback, void *user_data) {
    return pwm_monitor_create_with_name(pin, nullptr, callback, user_data);
}

PWMMonitor* pwm_monitor_create_with_name(int pin, const char *feature_name, PWMCallback callback, void *user_data) {
    if (!initialized || !chip) {
        LOG_ERROR(LOG_GPIO, "GPIO not initialized");
        return nullptr;
    }
    
    if (pin < 0 || pin > 27) {
        LOG_ERROR(LOG_GPIO, "Invalid pin number %d (must be 0-27)", pin);
        return nullptr;
    }
    
    PWMMonitor *monitor = calloc(1, sizeof(PWMMonitor));
    if (!monitor) {
        LOG_ERROR(LOG_GPIO, "Cannot allocate memory for PWM monitor");
        return nullptr;
    }
    
    monitor->pin = pin;
    monitor->feature_name = feature_name ? strdup(feature_name) : nullptr;
    monitor->callback = callback;
    monitor->user_data = user_data;
    monitor->active = false;
    monitor->has_new_reading = false;
    monitor->first_signal_received = false;
    monitor->waiting_for_fall = false;
    monitor->avg_window_ms = 200;
    monitor->sample_head = 0;
    
    // Clear sample buffer
    memset(monitor->samples, 0, sizeof(monitor->samples));
    
    mtx_init(&monitor->mutex, mtx_plain);
    
    // Get GPIO line for this pin
    struct gpiod_line *line = gpiod_chip_get_line(chip, pin);
    if (!line) {
        LOG_ERROR(LOG_GPIO, "Failed to get GPIO line %d", pin);
        mtx_destroy(&monitor->mutex);
        free(monitor->feature_name);
        free(monitor);
        return nullptr;
    }
    
    // Create request for edge events
    struct gpiod_line_request_config config = {
        .consumer = "helifx-pwm",
        .request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES,
        .flags = 0
    };
    
    monitor->line_request = gpiod_line_request_new(chip, line, &config);
    if (!monitor->line_request) {
        LOG_ERROR(LOG_GPIO, "Failed to request edge events for pin %d: %s", pin, strerror(errno));
        mtx_destroy(&monitor->mutex);
        free(monitor->feature_name);
        free(monitor);
        return nullptr;
    }
    
    LOG_INFO(LOG_GPIO, "PWM monitor created for [%s] pin %d", 
            feature_name ?: "Unknown", pin);
    return monitor;
}

void pwm_monitor_destroy(PWMMonitor *monitor) {
    if (!monitor) return;
    
    if (monitor->active) {
        pwm_monitor_stop(monitor);
    }
    
    if (monitor->line_request) {
        gpiod_line_request_release(monitor->line_request);
    }
    
    mtx_destroy(&monitor->mutex);
    
    if (monitor->feature_name) {
        free(monitor->feature_name);
    }
    
    free(monitor);
    
    LOG_INFO(LOG_GPIO, "PWM monitor destroyed");
}

int pwm_monitor_start(PWMMonitor *monitor) {
    if (!monitor) return -1;
    
    if (monitor->active) {
        LOG_WARN(LOG_GPIO, "PWM monitor already running");
        return 0;
    }
    
    // Add monitor to active list
    mtx_lock(&pwm_monitors_mutex);
    
    int slot = -1;
    for (int i = 0; i < MAX_PWM_MONITORS; i++) {
        if (!active_monitors[i]) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        mtx_unlock(&pwm_monitors_mutex);
        LOG_ERROR(LOG_GPIO, "Maximum number of PWM monitors (%d) reached", MAX_PWM_MONITORS);
        return -1;
    }
    
    active_monitors[slot] = monitor;
    active_monitor_count++;
    monitor->active = true;
    
    // Start shared monitoring thread if not running
    if (!pwm_thread_running) {
        pwm_thread_running = true;
        if (thrd_create(&pwm_monitoring_thread, pwm_monitoring_thread_func, NULL) != thrd_success) {
            LOG_ERROR(LOG_GPIO, "Failed to create PWM monitoring thread");
            active_monitors[slot] = NULL;
            active_monitor_count--;
            monitor->active = false;
            pwm_thread_running = false;
            mtx_unlock(&pwm_monitors_mutex);
            return -1;
        }
    }
    
    mtx_unlock(&pwm_monitors_mutex);
    
    LOG_INFO(LOG_GPIO, "PWM monitor started for [%s] pin %d (shared thread, %d active)", 
            monitor->feature_name ?: "Unknown", monitor->pin, active_monitor_count);
    return 0;
}

int pwm_monitor_stop(PWMMonitor *monitor) {
    if (!monitor) return -1;
    
    if (!monitor->active) {
        return 0;
    }
    
    // Remove monitor from active list
    mtx_lock(&pwm_monitors_mutex);
    
    for (int i = 0; i < MAX_PWM_MONITORS; i++) {
        if (active_monitors[i] == monitor) {
            active_monitors[i] = NULL;
            active_monitor_count--;
            break;
        }
    }
    
    monitor->active = false;
    
    // If no more active monitors, stop the shared thread
    if (active_monitor_count == 0 && pwm_thread_running) {
        pwm_thread_running = false;
        mtx_unlock(&pwm_monitors_mutex);
        thrd_join(pwm_monitoring_thread, NULL);
        mtx_lock(&pwm_monitors_mutex);
    }
    
    mtx_unlock(&pwm_monitors_mutex);
    
    LOG_INFO(LOG_GPIO, "PWM monitor stopped for [%s] (%d active)", 
            monitor->feature_name ?: "Unknown", active_monitor_count);
    return 0;
}

bool pwm_monitor_get_reading(PWMMonitor *monitor, PWMReading *reading) {
    if (!monitor || !reading) return false;
    
    mtx_lock(&monitor->mutex);
    
    bool has_reading = monitor->has_new_reading;
    if (has_reading) {
        *reading = monitor->current_reading;
        monitor->has_new_reading = false;
    }
    
    mtx_unlock(&monitor->mutex);
    
    return has_reading;
}

bool pwm_monitor_wait_reading(PWMMonitor *monitor, PWMReading *reading, int timeout_ms) {
    if (!monitor || !reading) return false;
    
    // With pigpio alerts, we don't have blocking wait anymore
    // This function now polls with timeout
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    while (1) {
        mtx_lock(&monitor->mutex);
        bool has_reading = monitor->has_new_reading;
        if (has_reading) {
            *reading = monitor->current_reading;
            monitor->has_new_reading = false;
            mtx_unlock(&monitor->mutex);
            return true;
        }
        mtx_unlock(&monitor->mutex);
        
        if (timeout_ms >= 0) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 + 
                             (now.tv_nsec - start.tv_nsec) / 1000000;
            if (elapsed_ms >= timeout_ms) {
                return false; // Timeout
            }
        }
        
        usleep(1000); // Sleep 1ms between checks
    }
}

bool pwm_monitor_is_running(PWMMonitor *monitor) {
    if (!monitor) return false;
    return monitor->active;
}

void pwm_monitor_set_avg_window_ms(PWMMonitor *monitor, int window_ms) {
    if (!monitor) return;
    if (window_ms < 10) window_ms = 10;
    if (window_ms > 5000) window_ms = 5000;
    mtx_lock(&monitor->mutex);
    monitor->avg_window_ms = window_ms;
    mtx_unlock(&monitor->mutex);
}

bool pwm_monitor_get_average(PWMMonitor *monitor, int *avg_us) {
    if (!monitor || !avg_us) return false;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long long window_ns;
    mtx_lock(&monitor->mutex);
    window_ns = (long long)monitor->avg_window_ms * 1000000LL;
    long long sum = 0;
    int count = 0;
    for (int i = 0; i < PWM_AVG_MAX_SAMPLES; i++) {
        struct timespec ts = monitor->samples[i].ts;
        if (ts.tv_sec == 0 && ts.tv_nsec == 0) continue;
        long long age_ns = ((long long)(now.tv_sec - ts.tv_sec) * 1000000000LL) + (now.tv_nsec - ts.tv_nsec);
        if (age_ns >= 0 && age_ns <= window_ns) {
            sum += monitor->samples[i].duration_us;
            count++;
        }
    }
    if (count == 0) {
        mtx_unlock(&monitor->mutex);
        return false;
    }
    *avg_us = (int)(sum / count);
    mtx_unlock(&monitor->mutex);
    return true;
}

bool gpio_is_initialized(void) {
    return initialized;
}
