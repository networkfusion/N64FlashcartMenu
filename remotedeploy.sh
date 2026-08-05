#!/bin/bash

set -e

REMOTE="--remote ${REMOTE:-host.docker.internal:9064}"
# Note: if host.docker.internal doesn't resolve, set REMOTE=172.20.0.1:9064 (WSL vEthernet adapter IP)

sc64deployer $REMOTE upload ./output/N64FlashcartMenu.n64

if [ "$1" = "-d" ]; then
    sc64deployer $REMOTE debug --no-writeback
fi

if [ "$1" = "-dr" ]; then
    sc64deployer $REMOTE debug --no-writeback --init "reboot"
fi

if [ "$1" = "-dur" ]; then
    sc64deployer $REMOTE debug --no-writeback --init "send-file /sc64menu.n64 @output/sc64menu.n64@;reboot"
fi
