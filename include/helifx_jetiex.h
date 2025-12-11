#ifndef HELIFX_JETIEX_H
#define HELIFX_JETIEX_H

#ifdef ENABLE_JETIEX

#include "jetiex.h"
#include "config_loader.h"
#include "gun_fx.h"
#include "engine_fx.h"

/**
 * @file helifx_jetiex.h
 * @brief JetiEX telemetry integration for HeliFX
 * 
 * This module integrates JetiEX bidirectional telemetry with the HeliFX system,
 * providing real-time sensor data and remote configuration capabilities.
 */

/**
 * @brief Initialize JetiEX telemetry for HeliFX
 * 
 * Creates and configures JetiEX telemetry with sensors and parameters.
 * Sets up callbacks for parameter changes and configuration saving.
 * 
 * @param config HeliFX configuration
 * @param config_file_path Path to config file for saving
 * @param gun Gun FX controller instance
 * @param engine Engine FX controller instance
 * @return JetiEX instance or nullptr on failure
 */
JetiEX* helifx_jetiex_init(HeliFXConfig *config, const char *config_file_path,
                           GunFX *gun, EngineFX *engine);

/**
 * @brief Update JetiEX telemetry sensors
 * 
 * Updates all telemetry sensors with current values from gun and engine controllers.
 * Should be called periodically in the main loop.
 * 
 * @param jetiex JetiEX instance
 * @param gun Gun FX controller instance
 * @param engine Engine FX controller instance
 */
void helifx_jetiex_update(JetiEX *jetiex, GunFX *gun, EngineFX *engine);

/**
 * @brief Cleanup JetiEX telemetry
 * 
 * Stops telemetry and destroys JetiEX instance.
 * 
 * @param jetiex JetiEX instance
 */
void helifx_jetiex_cleanup(JetiEX *jetiex);

#endif // ENABLE_JETIEX

#endif // HELIFX_JETIEX_H
