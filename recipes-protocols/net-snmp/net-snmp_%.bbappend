FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Explicitly track and include your physical snmpd.conf asset
SRC_URI += " \
    file://snmpd.conf \
"

do_install:append() {
    # Install the physical configuration file straight into the target rootfs
    install -d ${D}${sysconfdir}/snmp
    install -m 0600 ${WORKDIR}/snmpd.conf ${D}${sysconfdir}/snmp/snmpd.conf
}
