# Synchronize the user-space applications with the bleeding-edge libcamera API
SRC_URI = "git://github.com/raspberrypi/libcamera-apps.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"

# Update the package version string to reflect the latest commit
PV = "1.4.2+git${SRCPV}"

# 1. Annihilate the legacy boolean flags appended by the base meta-raspberrypi recipe
EXTRA_OEMESON:remove = "-Denable_drm=true -Denable_egl=false -Denable_libav=false -Denable_opencv=false -Denable_qt=false -Denable_tflite=false"

# 2. Inject the correctly typed strict combo options required by the main branch API
EXTRA_OEMESON:append = " -Denable_drm=enabled -Denable_egl=disabled -Denable_libav=disabled -Denable_opencv=disabled -Denable_qt=disabled -Denable_tflite=disabled"

# 3. Map the newly introduced dynamic plugins and tuning assets to the primary package payload
FILES:${PN} += " \
    ${libdir}/rpicam-apps-preview/* \
    ${libdir}/rpicam-apps-postproc/* \
    ${datadir}/rpi-camera-assets/* \
"

# 4. Silence benign host-path pollution warning in the development pkg-config metadata
INSANE_SKIP:${PN}-dev += "buildpaths"
