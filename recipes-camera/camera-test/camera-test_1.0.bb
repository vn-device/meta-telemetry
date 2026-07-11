SUMMARY = "Camera test python script"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de2b420"

SRC_URI = "file://camera_test.py"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/camera_test.py ${D}${bindir}/camera_test
}

FILES:${PN} = "${bindir}/camera_test"
