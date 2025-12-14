#!/bin/bash
#
# HeliFX Audio Setup Script
# Sets ALSA mixer levels for WM8960 Audio HAT or DigiAMP+
# 
# Usage:
#   sudo ./setup-audio-levels.sh [--card CARD_NAME]
#
# To run at startup, copy to /usr/local/bin/ and add to systemd:
#   sudo cp setup-audio-levels.sh /usr/local/bin/helifx-audio-setup
#   sudo chmod +x /usr/local/bin/helifx-audio-setup
#   sudo systemctl enable helifx-audio.service

set -e

# Default card name (auto-detect if not specified)
CARD_NAME=""
VERBOSE=0

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --card)
            CARD_NAME="$2"
            shift 2
            ;;
        --verbose|-v)
            VERBOSE=1
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [--card CARD_NAME] [--verbose]"
            echo ""
            echo "Options:"
            echo "  --card NAME    Specify ALSA card name (auto-detect if not provided)"
            echo "  --verbose      Show detailed output"
            echo "  --help         Show this help message"
            echo ""
            echo "Supported cards: wm8960-soundcard, iqaudiodacplus, Headphones"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Function to log messages
log() {
    echo "[AUDIO-SETUP] $1"
}

log_verbose() {
    if [ $VERBOSE -eq 1 ]; then
        echo "[AUDIO-SETUP] $1"
    fi
}

# Function to detect audio card
detect_card() {
    log "Detecting audio card..."
    
    # Try WM8960
    if aplay -l 2>/dev/null | grep -q "wm8960"; then
        echo "wm8960-soundcard"
        return 0
    fi
    
    # Try DigiAMP+ (iqaudio)
    if aplay -l 2>/dev/null | grep -q -i "iqaudio"; then
        echo "iqaudiodacplus"
        return 0
    fi
    
    # Fallback to Headphones (Pi onboard or any available card)
    if aplay -l 2>/dev/null | grep -q "Headphones"; then
        echo "Headphones"
        return 0
    fi
    
    # No recognized card found
    echo "unknown"
    return 1
}

# Function to set WM8960 levels
setup_wm8960() {
    log "Configuring WM8960 Audio HAT..."
    
    # Set card name
    local CARD="${1:-wm8960-soundcard}"
    
    # Unmute and set playback volume (0-255, using 255 = 100%)
    amixer -c "$CARD" sset 'Headphone',0 255 unmute >/dev/null 2>&1 || log_verbose "Could not set Headphone volume"
    amixer -c "$CARD" sset 'Speaker',0 255 unmute >/dev/null 2>&1 || log_verbose "Could not set Speaker volume"
    
    # Set PCM playback volume (0-255, using 255 = 100%)
    amixer -c "$CARD" sset 'PCM',0 255 >/dev/null 2>&1 || log_verbose "Could not set PCM volume"
    
    # Boost settings (optional, comment out if too loud)
    amixer -c "$CARD" sset 'Left Output Mixer PCM',0 on >/dev/null 2>&1 || log_verbose "Could not set Left Output Mixer"
    amixer -c "$CARD" sset 'Right Output Mixer PCM',0 on >/dev/null 2>&1 || log_verbose "Could not set Right Output Mixer"
    
    # Disable unused inputs (reduces noise)
    amixer -c "$CARD" sset 'Capture',0 0 mute >/dev/null 2>&1 || log_verbose "Could not mute capture"
    
    log "WM8960 configured: Headphone=100%, Speaker=100%, PCM=100%"
}

# Function to set DigiAMP+ levels
setup_digiamp() {
    log "Configuring Raspberry Pi DigiAMP+..."
    
    # Set card name
    local CARD="${1:-iqaudiodacplus}"
    
    # DigiAMP+ uses Digital volume control (0-207, 207=0dB, 206=-0.5dB, etc.)
    # Set to 75% volume (155 = ~-26dB, safer level for powerful amplifier)
    amixer -c "$CARD" sset 'Digital',0 155 >/dev/null 2>&1 || log_verbose "Could not set Digital volume"
    
    # Some DigiAMP+ models have additional controls
    amixer -c "$CARD" sset 'Playback',0 155 >/dev/null 2>&1 || log_verbose "Could not set Playback volume"
    
    # Ensure unmuted
    amixer -c "$CARD" sset 'Digital',0 unmute >/dev/null 2>&1 || log_verbose "Could not unmute Digital"
    
    log "DigiAMP+ configured: Digital=75% (~-26dB)"
}

# Function to set generic card levels
setup_generic() {
    log "Configuring generic audio device..."
    
    local CARD="${1:-Headphones}"
    
    # Try to set common control names
    amixer -c "$CARD" sset 'Master',0 75% unmute >/dev/null 2>&1 || log_verbose "Could not set Master volume"
    amixer -c "$CARD" sset 'PCM',0 100% unmute >/dev/null 2>&1 || log_verbose "Could not set PCM volume"
    amixer -c "$CARD" sset 'Headphone',0 75% unmute >/dev/null 2>&1 || log_verbose "Could not set Headphone volume"
    
    log "Generic card configured: Master=75%, PCM=100%"
}

# Main execution
main() {
    log "HeliFX Audio Level Setup"
    
    # Check if running as root (needed for some mixer operations)
    if [ "$EUID" -ne 0 ]; then 
        log "Warning: Not running as root. Some mixer controls may fail."
    fi
    
    # Detect or use specified card
    if [ -z "$CARD_NAME" ]; then
        CARD_NAME=$(detect_card)
        if [ "$CARD_NAME" = "unknown" ]; then
            log "ERROR: No supported audio card detected!"
            log "Available cards:"
            aplay -l 2>/dev/null || log "Could not list audio devices"
            exit 1
        fi
        log "Detected: $CARD_NAME"
    else
        log "Using specified card: $CARD_NAME"
    fi
    
    # Configure based on card type
    case "$CARD_NAME" in
        wm8960*|WM8960*)
            setup_wm8960 "$CARD_NAME"
            ;;
        iqaudio*|IQaudio*|DigiAMP*)
            setup_digiamp "$CARD_NAME"
            ;;
        *)
            setup_generic "$CARD_NAME"
            ;;
    esac
    
    # Show current mixer settings if verbose
    if [ $VERBOSE -eq 1 ]; then
        log "Current mixer settings:"
        amixer -c "$CARD_NAME" 2>/dev/null || log "Could not display mixer settings"
    fi
    
    log "Audio setup complete!"
}

# Run main function
main
