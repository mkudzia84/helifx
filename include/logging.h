#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>

/**
 * @file logging.h
 * @brief Unified logging macros for consistent output across all components
 * 
 * This header provides standardized logging macros for error, warning, and
 * info messages with component-based tagging for easy filtering and debugging.
 */

/* Color codes for terminal output (optional, can be disabled) */
#define LOG_COLOR_RED     "\033[91m"
#define LOG_COLOR_YELLOW  "\033[93m"
#define LOG_COLOR_GREEN   "\033[92m"
#define LOG_COLOR_BLUE    "\033[94m"
#define LOG_COLOR_RESET   "\033[0m"

/* Component prefixes */
#define LOG_HELIFX   "[HELIFX] "
#define LOG_CONFIG   "[CONFIG] "
#define LOG_ENGINE   "[ENGINE] "
#define LOG_GUN      "[GUN]    "
#define LOG_SERVO    "[SERVO]  "
#define LOG_AUDIO    "[AUDIO]  "
#define LOG_SMOKE    "[SMOKE]  "
#define LOG_GPIO     "[GPIO]   "
#define LOG_DEMO     "[DEMO]   "

/* Log level macros - Standard output to stdout/stderr */

/**
 * @brief Log an error message (stderr, with component prefix)
 * @param component Component prefix (e.g., LOG_GUN)
 * @param fmt Format string
 * @param ... Arguments
 */
#define LOG_ERROR(component, fmt, ...) \
    fprintf(stderr, component "Error: " fmt "\n", ##__VA_ARGS__)

/**
 * @brief Log a warning message (stderr, with component prefix)
 * @param component Component prefix (e.g., LOG_GUN)
 * @param fmt Format string
 * @param ... Arguments
 */
#define LOG_WARN(component, fmt, ...) \
    fprintf(stderr, component "Warning: " fmt "\n", ##__VA_ARGS__)

/**
 * @brief Log informational message (stdout, with component prefix)
 * @param component Component prefix (e.g., LOG_GUN)
 * @param fmt Format string
 * @param ... Arguments
 */
#define LOG_INFO(component, fmt, ...) \
    printf(component fmt "\n", ##__VA_ARGS__)

/**
 * @brief Log debug/verbose message (stdout, with component prefix)
 * Only compiled if DEBUG is defined at compile time
 * @param component Component prefix (e.g., LOG_GUN)
 * @param fmt Format string
 * @param ... Arguments
 */
#ifdef DEBUG
#define LOG_DEBUG(component, fmt, ...) \
    printf(component "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG_DEBUG(component, fmt, ...) ((void)0)
#endif

/**
 * @brief Log a status line (for periodic dumps)
 * Status lines are untagged for compact display
 * @param fmt Format string
 * @param ... Arguments
 */
#define LOG_STATUS(fmt, ...) \
    printf(fmt "\n", ##__VA_ARGS__)

/**
 * @brief Log initialization message
 * @param component Component prefix
 * @param details Description of what was initialized
 */
#define LOG_INIT(component, details) \
    LOG_INFO(component, "Initialized: %s", details)

/**
 * @brief Log shutdown message
 * @param component Component prefix
 * @param details Description of what was shut down
 */
#define LOG_SHUTDOWN(component, details) \
    LOG_INFO(component, "Shutdown: %s", details)

/**
 * @brief Log state change message
 * @param component Component prefix
 * @param from_state Previous state
 * @param to_state New state
 */
#define LOG_STATE(component, from_state, to_state) \
    LOG_INFO(component, "State: %s → %s", from_state, to_state)

/**
 * @brief Log a configuration item during startup
 * @param component Component prefix
 * @param item Configuration item name
 * @param value Configuration value
 */
#define LOG_CONFIG_ITEM(component, item, value) \
    printf(component "  %s: %s\n", item, value)

#endif // LOGGING_H
