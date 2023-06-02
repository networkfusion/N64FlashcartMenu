#!/bin/bash

set -e

make -j

REMOTE="--remote host.docker.internal:9064"

sc64deployer $REMOTE upload N64FlashcartMenu.z64

if [ "$1" = "-d" ]; then
    sc64deployer $REMOTE debug
fi
