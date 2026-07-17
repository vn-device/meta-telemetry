SUMMARY = "C++ WebSocket Telemetry Daemon"
DESCRIPTION = "Persistent headless backend for hardware interrupts and IPC"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit qt6-cmake systemd pkgconfig

DEPENDS = "qtbase qtwebsockets libgpiod"

SYSTEMD_SERVICE:${PN} = "telemetry-daemon.service"
SYSTEMD_AUTO_ENABLE = "enable"

SRC_URI = " \
    file://CMakeLists.txt \
    file://main.cpp \
    file://TelemetryServer.h \
    file://TelemetryServer.cpp \
    file://HardwareMonitor.h \
    file://HardwareMonitor.cpp \
    file://telemetry-daemon.service \
"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/telemetry_daemon ${D}${bindir}/telemetry_daemon

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/telemetry-daemon.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${bindir}/telemetry_daemon"