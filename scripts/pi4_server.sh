#!/bin/bash

RED='\033[1;31m'
PURPLE='\033[0;35m'
BLUE='\033[0;36m'
NC='\033[0m'

MODE=$1

if [ $# -eq 1 ]; then

    if [ "${MODE}" = "start" ]; then

        printf "${PURPLE}"
        printf "\n================================================================================\n"
        printf ">>>>> pi4_server.sh start <<<<<\n"
        printf "${NC}"

        printf "${BLUE}"
        printf "[1/2] starting necessary services...\n"
        printf "${NC}"
        sudo modprobe usbip_host
        sudo usbipd &

        printf "${BLUE}"
        printf "[2/2] listing available devices...\n"
        printf "${NC}"
        DEVICE_ARRAY=$(sudo usbip list -l | cut -d " " -f 4 | grep [0-9])
        for DEVICE in $DEVICE_ARRAY; do
            printf "bind busid '$DEVICE' \n"
            sudo usbip bind -b $DEVICE
        done

        printf "${PURPLE}"
        printf "================================================================================\n"
        printf "${NC}"

    elif [ "${MODE}" = "stop" ]; then

        printf "${PURPLE}"
        printf "\n================================================================================\n"
        printf ">>>>> pi4_server.sh stop <<<<<\n"
        printf "${NC}"

        printf "${BLUE}"
        printf "Detaching available devices \n"
        printf "${NC}"
        DEVICE_ARRAY=$(sudo usbip list -l | cut -d " " -f 4 | grep [0-9])
        for DEVICE in $DEVICE_ARRAY; do
            printf "unbind busid '$DEVICE' \n"
            sudo usbip unbind -b $DEVICE
        done

        sudo killall -9 usbipd

        printf "${PURPLE}"
        printf "================================================================================\n"
        printf "${NC}"

    else

        printf "${RED}"
        printf "argument 1 is expected to be : {start/stop}\n"
        printf "${NC}"
        exit 1

    fi

else

    printf "${RED}"
    printf "1 argument is expected : {start/stop}\n"
    printf "${NC}"
    exit 1

fi
