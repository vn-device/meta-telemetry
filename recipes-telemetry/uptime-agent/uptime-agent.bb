SUMMARY = "Custom AgentX Telemetry Subagent"
DESCRIPTION = "A standalone Net-SNMP subagent to track internal application uptime."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://main.c \
    file://appUptimeSeconds.c \
    file://appUptimeSeconds.h \
    file://uptime-agent.service \
"

S = "${WORKDIR}"

# Require Net-SNMP in the cross-compilation sysroot to access the AgentX headers
DEPENDS = "net-snmp"

# Inherit systemd class for automatic service registration during rootfs generation
inherit systemd

SYSTEMD_SERVICE:${PN} = "uptime-agent.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_compile() {
    # Include CFLAGS to ensure Raspberry Pi 5 target-specific optimization and hardware flags are passed
    # Explicitly point the cross-compiler to the Yocto staging directories for Net-SNMP headers and shared libraries
    ${CC} ${CFLAGS} main.c appUptimeSeconds.c -o uptime-agent \
        -I${STAGING_INCDIR} \
        -L${STAGING_LIBDIR} \
        ${LDFLAGS} -lnetsnmpmibs -lnetsnmpagent -lnetsnmp
}

do_install() {
    # Install the compiled C binary into the target's /usr/bin/
    install -d ${D}${bindir}
    install -m 0755 uptime-agent ${D}${bindir}/

    # Install the systemd unit file into the target's /lib/systemd/system/
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/uptime-agent.service ${D}${systemd_system_unitdir}/
}