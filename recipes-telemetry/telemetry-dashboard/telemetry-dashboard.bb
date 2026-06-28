SUMMARY = "Recipe for modern WebSocket Hardware Telemetry Dashboard"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://dashboard.html"

S = "${WORKDIR}"

do_install() {
    install -d ${D}/www/pages
    install -m 0644 ${WORKDIR}/dashboard.html ${D}/www/pages/dashboard.html
}

FILES:${PN} += "/www/pages/dashboard.html"

# Force the core image to pull in lighttpd whenever the dashboard is built
RDEPENDS:${PN} += "lighttpd"