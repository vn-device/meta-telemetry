FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

do_install:append() {
    # Dynamically inject the AgentX IPC directive into the master snmpd.conf
    # This avoids overwriting the entire upstream configuration file
    echo "master agentx" >> ${D}${sysconfdir}/snmp/snmpd.conf
}
