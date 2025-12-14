#!/bin/bash
#
# HeliFX Audio Setup Script
# Sets ALSA mixer levels for WM8960 Audio HAT or DigiAMP+
# 
# Usage:
#   sudo ./setup-audio-levels.sh [--card 0|NAME]

set -e

CARD="0"
VERBOSE=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --card) CARD="$2"; shift 2 ;;
        --verbose|-v) VERBOSE=1; shift ;;
        --help|-h)
            echo "Usage: $0 [--card 0|NAME] [--verbose]"
            echo "  --card     ALSA card number or name (default: 0)"
            echo "  --verbose  Show detailed output"
            exit 0
            ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

log() { echo "[AUDIO-SETUP] $1"; }
log_verbose() { [ $VERBOSE -eq 1 ] && echo "[AUDIO-SETUP] $1"; }

log "HeliFX Audio Level Setup"

# Detect card type
CARD_NAME=$(aplay -l 2>/dev/null | grep "^card $CARD" | grep -o '\[.*\]' | tr -d '[]' || echo "unknown")
log_verbose "Card name: $CARD_NAME"

# Set volume based on card type
if echo "$CARD_NAME" | grep -qi "wm8960"; then
    VOLUME="100%"
    log "Detected WM8960 - setting to 100%"
else
    VOLUME="75%"
    log "Setting levels to 75%"
fi

# Set mixer controls
set_ctrl() {
    amixer -c "$CARD" sset "$1" "$2" unmute >/dev/null 2>&1
}

# Set controls
set_ctrl 'Headphone' "$VOLUME" || log_verbose "Headphone control not available"
set_ctrl 'Speaker' "$VOLUME" || log_verbose "Speaker control not available"
set_ctrl 'Playback' "$VOLUME" || log_verbose "Playback control not available"
set_ctrl 'Digital' "$VOLUME" || log_verbose "Digital control not available"

# Optional: Enable output mixers for WM8960
amixer -c "$CARD" sset 'Left Output Mixer PCM' on >/dev/null 2>&1 || true
amixer -c "$CARD" sset 'Right Output Mixer PCM' on >/dev/null 2>&1 || true

# Optional: Mute capture to reduce noise
amixer -c "$CARD" sset 'Capture' 0 mute >/dev/null 2>&1 || true

log "Audio setup complete!"
