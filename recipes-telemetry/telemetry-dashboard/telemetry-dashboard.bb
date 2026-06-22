SUMMARY = "HTML Telemetry Dashboard and CGI Bridge"
DESCRIPTION = "Provides the index.cgi script to bridge SNMPv3 data to Lighttpd."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://index.cgi"

S = "${WORKDIR}"

# Inherit allarch since this package only deploys an architecture-independent script.
# This optimizes build times by bypassing architecture-specific compilation tasks.
inherit allarch

# Enforce strict runtime dependencies for the presentation layer
RDEPENDS:${PN} = "lighttpd lighttpd-module-cgi bash net-snmp-client"

do_install() {
    # Generate the web root directory
    install -d ${D}/www/pages
    
    # Deploy the CGI script with execution permissions required by Lighttpd
    install -m 0755 ${WORKDIR}/index.cgi ${D}/www/pages/index.cgi
}

FILES:${PN} += "/www/pages/index.cgi"