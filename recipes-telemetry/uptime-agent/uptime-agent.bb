SUMMARY = "Custom AgentX Telemetry Subagent"
DESCRIPTION = "A standalone Net-SNMP subagent to track internal application uptime."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Instruct BitBake to pull these specific files from the 'files' subdirectory
SRC_URI = " \
    file://main.c \
    file://appUptimeSeconds.c \
    file://appUptimeSeconds.h \
    file://uptime-agent.service \
"

# Set the working directory to where the files are unpacked
S = "${WORKDIR}"

# Require Net-SNMP in the cross-compilation sysroot to access the AgentX headers
DEPENDS = "net-snmp"

# Inherit systemd class to automatically handle service enablement during rootfs generation
inherit systemd

SYSTEMD_SERVICE:${PN} = "uptime-agent.service"
SYSTEMD_AUTO_ENABLE = "enable"

# The cross-compilation phase
do_compile() {
    # ${CC} dynamically resolves to the ARM64 cross-compiler (aarch64-poky-linux-gcc)
    # -I and -L explicitly point the compiler to the Yocto staging directories for Net-SNMP
    ${CC} main.c appUptimeSeconds.c -o uptime-agent \
        -I${STAGING_INCDIR} \
        -L${STAGING_LIBDIR} \
        ${LDFLAGS} -lnetsnmpmibs -lnetsnmpagent -lnetsnmp
}

# The deployment phase into the target filesystem
do_install() {
    # Install the executable into /usr/bin/
    install -d ${D}${bindir}
    install -m 0755 uptime-agent ${D}${bindir}/

    # Install the systemd unit file into /lib/systemd/system/
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/uptime-agent.service ${D}${systemd_system_unitdir}/
}
