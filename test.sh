#!/bin/bash
IDEVER="1.8.7"
TEENSYVER="1.44"
WORKDIR="/tmp/autobuild_$$"
mkdir -p ${WORKDIR}
# Install Ardino IDE with teensyduino in work directory
TARFILE="${HOME}/Downloads/arduino-${IDEVER}-teensyduino-1.44.tbz"
if [ -f ${TARFILE} ]
then
    tar xf ${TARFILE} -C ${WORKDIR}
else
    exit -1
fi
# Create portable sketchbook and library directories
IDEDIR="${WORKDIR}/arduino-${IDEVER}"
LIBDIR="${IDEDIR}/portable/sketchbook/libraries"
mkdir -p "${LIBDIR}"
export PATH="${IDEDIR}:${PATH}"
cd ${IDEDIR}
which arduino
# Configure board package
BOARD="teensy:avr:teensy36"
arduino --pref "compiler.warning_level=default" \
        --pref "custom_usb=teensy36_serialhid" \
        --pref "custom_keys=teensy36_en-us" \
        --pref "custom_opt=teensy36_o2std" \
        --pref "custom_speed=teensy36_180" \
        --pref "update.check=false" \
        --pref "editor.external=true" --save-prefs
arduino --board "${BOARD}" --save-prefs
CC="arduino --verify --board ${BOARD}"
cd ${LIBDIR}
if [ -d ${HOME}/Sync/IntelliKeys_t36/ ]
then
    ln -s ${HOME}/Sync/IntelliKeys_t36/
else
    git clone https://github.com/gdsports/IntelliKeys_t36
fi
cd IntelliKeys_t36/examples/IntelliKeys
$CC IntelliKeys.ino
