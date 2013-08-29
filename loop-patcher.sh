#!/bin/bash
echo $BASHPID > /tmp/patcher.pid
while true; do
    ./patcher
    sleep 1;
done
