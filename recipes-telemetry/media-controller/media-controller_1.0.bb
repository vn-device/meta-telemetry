SUMMARY = "C++ Media Controller Daemon"
DESCRIPTION = "Backend service for IMX708 ISP bindings and WebSocket media routing"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit qt6-cmake systemd pkgconfig

# Add libcamera to the dependency graph
DEPENDS = "qtbase qtwebsockets libcamera"

SYSTEMD_SERVICE:${PN} = "media-controller.service"
SYSTEMD_AUTO_ENABLE = "enable"

SRC_URI = " \
    file://CMakeLists.txt \
    file://main.cpp \
    file://MediaController.h \
    file://MediaController.cpp \
    file://media-controller.service \
"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/media_controller ${D}${bindir}/media_controller

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/media-controller.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${bindir}/media_controller"