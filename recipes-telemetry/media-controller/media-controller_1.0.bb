SUMMARY = "Media Controller Daemon for IMX708 Camera"
DESCRIPTION = "A Qt6 WebSocket server managing the camera ISP pipeline and streaming MJPEG frames."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "qtbase qtwebsockets"

SRC_URI = " \
    file://media-controller.pro \
    file://main.cpp \
    file://MediaController.cpp \
    file://MediaController.h \
    file://media-controller.service \
"

S = "${WORKDIR}"

inherit qt6-qmake systemd

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN} = "media-controller.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install() {
    # Install the compiled binary
    install -d ${D}${bindir}
    install -m 0755 ${B}/media-controller ${D}${bindir}/

    # Install the systemd service file
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/media-controller.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${systemd_system_unitdir}/media-controller.service"
