SUMMARY = "HTML Telemetry Dashboard"
DESCRIPTION = "Provides a modern WebSocket-based dashboard for hardware telemetry."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Track modular frontend assets
SRC_URI = " \
    file://dashboard.html \
    file://css/style.css \
    file://js/config.js \
    file://js/network.js \
    file://js/ui.js \
"

S = "${WORKDIR}"

inherit allarch

# Runtime dependencies: lighttpd serves static assets; 
# WebSocket communication is handled by the backend daemon.
RDEPENDS:${PN} = "lighttpd"

do_install() {
    # Generate web root directories
    install -d ${D}/www/pages/css
    install -d ${D}/www/pages/js
    
    # Deploy structural HTML
    install -m 0644 ${WORKDIR}/dashboard.html ${D}/www/pages/dashboard.html
    
    # Deploy Presentation and JS Logic mapping
    install -m 0644 ${WORKDIR}/css/style.css ${D}/www/pages/css/style.css
    install -m 0644 ${WORKDIR}/js/config.js ${D}/www/pages/js/config.js
    install -m 0644 ${WORKDIR}/js/network.js ${D}/www/pages/js/network.js
    install -m 0644 ${WORKDIR}/js/ui.js ${D}/www/pages/js/ui.js
}

# Ship all assets in the main package payload
FILES:${PN} += "/www/pages/*"