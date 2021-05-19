#!/bin/bash

#
# Copyright 2020 JDD authors.
# @yangchunxin3
#

if [ $# -lt 2 ]; then
    echo "Usage: $0 (make|make_install|clean) (all|client|access|setmanager|extentmanager|extentserver)"
    exit 1
fi

CMD=$1
MODULE=$2

function do_client() {
    echo "Begin ${CMD} client"
    cd clients/libcypre
    if [ ${CMD} = "make" ]; then
       make so
    elif [ ${CMD} = "make_install" ]; then
        make install
    else
        make clean
    fi
    echo "End ${CMD} client"
}

function do_access() {
    echo "Begin ${CMD} access"
    echo "End ${CMD} access"
}

function do_setmanager() {
    echo "Begin ${CMD} setmanager"
    cd src/setmanager
    if [ ${CMD} = "make" ]; then
        make -j4
    elif [ ${CMD} = "make_install" ]; then
        make install
    else
        make clean
    fi
    cd -
    echo "End ${CMD} setmanager"
}

function do_extentmanager() {
    echo "Begin ${CMD} extentmanager"
    cd src/extentmanager
    if [ ${CMD} = "make" ]; then
        make -j4
    elif [ ${CMD} = "make_install" ]; then
        make install
    else
        make clean
    fi
    cd -
    echo "End ${CMD} extentmanager"
}

function do_extentserver() {
    echo "Begin ${CMD} extentserver"
    cd src/extentserver
    if [ ${CMD} = "make" ]; then
        make -j4
    elif [ ${CMD} = "make_install" ]; then
        make install
    else
        make clean
    fi
    cd -
    echo "End ${CMD} extentserver"
}

case ${MODULE} in
    client)
        do_client
        ;;
    access)
        do_access
        ;;
    setmanager)
        do_setmanager
        ;;
    extentmanager)
        do_extentmanager
        ;;
    extentserver)
        do_extentserver
        ;;
    all)
        do_client
        do_access
        do_setmanager
        do_extentmanager
        do_extentserver
        ;;
    *)
        echo "Usage: $0 (all|client|access|setmanager|extentmanager|extentserver)"
        exit 1
esac
