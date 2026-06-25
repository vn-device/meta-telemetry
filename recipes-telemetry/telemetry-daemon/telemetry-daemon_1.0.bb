SUMMARY = "C++ WebSocket Telemetry Daemon"
DESCRIPTION = "Persistent headless backend for hardware interrupts and IPC"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit qt6-cmake systemd

DEPENDS = "qtbase qtwebsockets"

SRC_URI = " \
    file://CMakeLists.txt \
    file://main.cpp \
    file://TelemetryServer.h \
    file://TelemetryServer.cpp \
    file://telemetry-daemon.service \
"

S = "${WORKDIR}"

# Define the systemd service file and explicitly enable it on boot
SYSTEMD_SERVICE:${PN} = "telemetry-daemon.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install() {
    # Install the compiled C++ binary
    install -d ${D}${bindir}
    install -m 0755 telemetry_daemon ${D}${bindir}/telemetry_daemon

    # Install the systemd service unit
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/telemetry-daemon.service ${D}${systemd_system_unitdir}
}

FILES:${PN} += " \
    ${bindir}/telemetry_daemon \
    ${systemd_system_unitdir}/telemetry-daemon.service \
"