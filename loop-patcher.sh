#!/bin/bash
echo $BASHPID > /tmp/patcher.pid
while true; do
    t1=`date +%s`
    ./patcher
    t2=`date +%s`
    if [ `expr $t2 - $t1` -lt 3 ]; then
        # immediate crash, wait
        sleep 1;
    fi
done
