#!/bin/bash

set -e

REMOTE="--remote host.docker.internal:9064"

sc64deployer $REMOTE upload ./output/N64FlashcartMenu.z64

echo
echo
# Toggle the power of the N64 to boot the ROM.
echo !!! Now toggle power to the N64 !!!
echo
echo

if [ "$1" = "-d" ]; then
    sc64deployer $REMOTE debug
fi
