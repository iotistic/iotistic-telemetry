SUMMARY = "IOTISTIC Telemetry"
DESCRIPTION = "Builds M2NVR ${PN} service"
LICENSE = "CLOSED"

BB_DONT_CACHE = "1"

# Version read automatically from latest git tag
def read_git_version(d):
    import subprocess
    import os
    thisdir = d.getVar('THISDIR')
    try:
        version = subprocess.check_output(
            ['git', 'describe', '--tags', '--abbrev=0'],
            cwd=thisdir,
            stderr=subprocess.DEVNULL
        ).decode().strip().lstrip('v')
        return version
    except Exception:
        return "0.0.0"

PV = "${@read_git_version(d)}"

SRC_URI = "file://redis.cpp \
           file://redis.hpp \
           file://SqlDatabase.cpp \
           file://SqlDatabase.hpp \
           file://utils.cpp \
           file://utils.hpp \
           file://IOTISTIC_Messaging_Protocol.cpp \
           file://IOTISTIC_Messaging_Protocol.hpp \
           file://config.hpp \
           file://config.cpp \
           file://MQTTClientBase.hpp \
           file://MQTTClientBase.cpp \
           file://FlespiMQTT.hpp \
           file://FlespiMQTT.cpp \
           file://main.hpp \
           file://main.cpp \
           file://${PN}.service"

S = "${WORKDIR}"

DEPENDS = "paho-mqtt-c hiredis cjson sqlite3"

TARGET_CXXFLAGS += "-g -O0"
TARGET_LDFLAGS += "-g"

INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"

inherit systemd deploy

do_compile() {
    ${CXX} ${CXXFLAGS} ${LDFLAGS} \
        -I${S} \
        ${S}/redis.cpp \
        ${S}/SqlDatabase.cpp \
        ${S}/utils.cpp \
        ${S}/config.cpp \
        ${S}/IOTISTIC_Messaging_Protocol.cpp \
        ${S}/MQTTClientBase.cpp \
        ${S}/FlespiMQTT.cpp \
        ${S}/main.cpp \
        -o ${B}/${PN} \
        -std=c++17 \
        -lpaho-mqtt3c -lhiredis -lcjson -lsqlite3 -lpthread -lrt -lm
}

do_install() {
    # Install binary
    install -d ${D}${bindir}
    install -m 0755 ${B}/${PN} ${D}${bindir}

    # Install version file
    install -d ${D}/usr/share/iotistic-services
    echo "${PV}" > ${D}/usr/share/iotistic-services/${PN}.version

    # Install systemd service
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/${PN}.service ${D}${systemd_system_unitdir}
}

do_deploy() {
    install -m 0755 ${B}/${PN} ${DEPLOYDIR}/${PN}
    echo "${PV}" > ${DEPLOYDIR}/${PN}.version
}
addtask do_deploy after do_compile before do_build

FILES:${PN} = " \
    ${bindir}/${PN} \
    ${systemd_system_unitdir}/${PN}.service \
    /usr/share/iotistic-services/${PN}.version \
"

SYSTEMD_SERVICE:${PN} = "${PN}.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"