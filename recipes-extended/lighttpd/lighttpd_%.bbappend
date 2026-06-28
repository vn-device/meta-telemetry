FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " file://lighttpd.conf"

do_install:append() {
    # Delete any stock configuration file that might block the installation
    rm -f ${D}${sysconfdir}/lighttpd/lighttpd.conf

    # Forcefully install our specific, CGI-free template file
    install -d ${D}${sysconfdir}/lighttpd
    install -m 0644 ${THISDIR}/${PN}/lighttpd.conf ${D}${sysconfdir}/lighttpd/lighttpd.conf
}