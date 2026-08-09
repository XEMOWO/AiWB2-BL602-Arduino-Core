# Component makefile — lets the Ai-Thinker WB2 SDK (bl_iot_sdk) build this
# directory as a component named "arduino". The very same files are used as a
# standard Arduino core (cores/arduino) by the Arduino IDE board package.
#
# The board pin map lives in variants/<board>/pins_arduino.h, which lives
# OUTSIDE this component; export it here so every source (core + sketch) can
# `#include "pins_arduino.h"` through Arduino.h.

COMPONENT_ADD_INCLUDEDIRS += .
COMPONENT_ADD_INCLUDEDIRS += ../../variants/wb2-12f

# Sources: core files here, plus the board's variant.cpp (pin table defs)
COMPONENT_SRCDIRS := . ../../variants/wb2-12f
