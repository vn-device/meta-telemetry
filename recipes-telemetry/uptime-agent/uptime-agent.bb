SUMMARY = "Custom AgentX Telemetry Subagent"
DESCRIPTION = "A standalone Net-SNMP subagent to track internal application uptime."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://main.c \
    file://appUptimeSeconds.c \
    file://appUptimeSeconds.h \
    file://gpioTable.c \
    file://gpioTable.h \
    file://uptime-agent.service \
    file://LOCAL-TELEMETRY-MIB.txt \
"

S = "${WORKDIR}"

# Require Net-SNMP in the cross-compilation sysroot to access the AgentX headers
DEPENDS = "net-snmp libgpiod"

# Inherit systemd class for automatic service registration during rootfs generation
inherit systemd

SYSTEMD_SERVICE:${PN} = "uptime-agent.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

# Explicitly assign the deployed MIB text file to the primary binary package
FILES:${PN} += "${datadir}/snmp/mibs/LOCAL-TELEMETRY-MIB.txt"

do_compile() {
    # Include CFLAGS to ensure Raspberry Pi 5 target-specific optimization and hardware flags are passed
    # Explicitly point the cross-compiler to the Yocto staging directories for Net-SNMP headers and shared libraries
    ${CC} ${CFLAGS} main.c appUptimeSeconds.c gpioTable.c -o uptime-agent \
        -I${STAGING_INCDIR} \
        -L${STAGING_LIBDIR} \
        ${LDFLAGS} -lnetsnmpmibs -lnetsnmpagent -lnetsnmp -lgpiod
}

do_install() {
    # Install the compiled C binary into the target's /usr/bin/
    install -d ${D}${bindir}
    install -m 0755 uptime-agent ${D}${bindir}/

    # Install the systemd unit file into the target's /lib/systemd/system/
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/uptime-agent.service ${D}${systemd_system_unitdir}/

    # Construct the enterprise MIB directory and deploy the SMIv2 schema
    # Local files added to SRC_URI are staged in WORKDIR
    install -d ${D}${datadir}/snmp/mibs
    install -m 0644 ${WORKDIR}/LOCAL-TELEMETRY-MIB.txt ${D}${datadir}/snmp/mibs/
}