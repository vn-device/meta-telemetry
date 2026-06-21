# We do not need FILESEXTRAPATHS since we are not supplying a physical local file
# SRC_URI is omitted for the same reason

do_install:append() {
    # Dynamically inject the complete telemetry and security matrix into the master snmpd.conf
    # This preserves upstream defaults while unblocking the custom AgentX tree

    echo "" >> ${D}${sysconfdir}/snmp/snmpd.conf
    echo "# Custom Telemetry Subagent Configuration" >> ${D}${sysconfdir}/snmp/snmpd.conf
    
    # 1. Enable the IPC Socket
    echo "master agentx" >> ${D}${sysconfdir}/snmp/snmpd.conf
    
    # 2. Whitelist the Enterprise Tree in the VACM
    echo "view systemonly included .1.3.6.1.4.1.99999" >> ${D}${sysconfdir}/snmp/snmpd.conf
    
    # 3. V2c Debug Fallback (Required for your active simulation matrix)
    echo "rocommunity public default -V systemonly" >> ${D}${sysconfdir}/snmp/snmpd.conf
    
    # 4. V3 Production Security Matrix
    echo "createUser admin SHA \"authpassword123\" AES \"privpassword123\"" >> ${D}${sysconfdir}/snmp/snmpd.conf
    echo "rouser admin authPriv -V systemonly" >> ${D}${sysconfdir}/snmp/snmpd.conf
}