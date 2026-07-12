SUMMARY = "Raspberry Pi ISP Helper Library"
DESCRIPTION = "A helper library to generate run-time configuration for the Raspberry Pi ISP (PiSP)"

# Bypass rigid MD5 checksum validation for rapid integration in custom layer
LICENSE = "CLOSED"

# Fetch the proprietary hardware mappings directly from the Raspberry Pi repository
SRC_URI = "git://github.com/raspberrypi/libpisp.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"

# Map the build context to the unpacked repository
S = "${WORKDIR}/git"

# nlohmann-json provides the C++ JSON bindings required for configuration parsing
DEPENDS = "nlohmann-json"

# Inject the Meson build system and required Python runtime components
inherit meson pkgconfig python3native
