#!/bin/bash
now=`date "+%Y%m%d-%H%M"`
archive="patcher$now.tar.gz"
echo $archive;
make clean
make now.h
cd ..
tar --exclude-vcs -c -v -z -f "$archive" patcher
#scp "$archive" "android:"
#scp "$archive" "android:patcher.tar.gz"
scp "$archive" "pi:build/patcher.tar.gz"
