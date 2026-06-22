FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Track both the custom webserver configuration and the early-boot volatile media rules
SRC_URI += " \
    file://lighttpd.conf \
    file://lighttrap-tmpfiles.conf \
"

# Ship custom files in main package payload to bypass installed-vs-shipped QA check
FILES:${PN} += " \
    ${libdir}/tmpfiles.d/lighttpd.conf \
    ${sysconfdir}/systemd/system/lighttpd.service.d/override.conf \
    ${sysconfdir}/systemd/system/lighttpd.service.d \
"

do_install:append() {
    # 1. Deploy the custom CGI and port 80 routing parameters
    install -d ${D}${sysconfdir}/lighttpd
    install -m 0644 ${WORKDIR}/lighttpd.conf ${D}${sysconfdir}/lighttpd/lighttpd.conf

    # 2. Deploy the native systemd tmpfiles configuration (Aligned with Option A filename)
    install -d ${D}${libdir}/tmpfiles.d
    install -m 0644 ${WORKDIR}/lighttrap-tmpfiles.conf ${D}${libdir}/tmpfiles.d/lighttpd.conf

    # 3. Enforce synchronization parameters
    install -d ${D}${sysconfdir}/systemd/system/lighttpd.service.d
    echo "[Unit]" > ${D}${sysconfdir}/systemd/system/lighttpd.service.d/override.conf
    echo "Requires=systemd-tmpfiles-setup.service" >> ${D}${sysconfdir}/systemd/system/lighttpd.service.d/override.conf
    echo "After=systemd-tmpfiles-setup.service" >> ${D}${sysconfdir}/systemd/system/lighttpd.service.d/override.conf
}