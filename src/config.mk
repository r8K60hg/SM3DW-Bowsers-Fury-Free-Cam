#----------------------------- User configuration -----------------------------

# How the module is loaded (subsdk9 next to the game's own modules).
LOAD_KIND := Module

# Super Mario 3D World + Bowser's Fury
PROGRAM_ID := 010028600EBDA000

# Optional path to copy the final ELF to, for convenience.
ELF_EXTRACT :=

# Python command to use. Must be Python 3.4+.
PYTHON := python3

# JSON to use for `make npdm-json` (the build itself uses config.json).
NPDM_JSON := qlaunch.json

# Additional C/C++ flags. -Wno-error keeps a harmless new compiler warning from failing the build.
C_FLAGS := -Wno-error
CXX_FLAGS :=

# Module settings
#------------------------
FTP_IP := 192.168.0.235
FTP_PORT := 5000
FTP_USERNAME := anonymous
FTP_PASSWORD :=

RYU_PATH :=
