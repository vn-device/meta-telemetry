# Hijack the vanilla upstream source and point it directly to the Raspberry Pi vendor fork
SRC_URI = "git://github.com/raspberrypi/libcamera.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"

# Update the package version to reflect the git override
PV = "0.4.0+git${SRCPV}"

# Add the missing Pi 5 Image Signal Processor backend dependency
DEPENDS:append = " libpisp"

# Explicitly declare the hardware backend pipelines for the Pi 5 ISP
EXTRA_OEMESON:append = " -Dpipelines=rpi/vc4,rpi/pisp -Dipas=rpi/vc4,rpi/pisp"
