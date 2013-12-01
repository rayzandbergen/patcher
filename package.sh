#!/bin/bash
now=`date "+%Y%m%d-%H%M"`
archive="patcher$now.tar.gz"
echo $archive;
make clean
rm -rf doc tags fantom_cache.bin CMakeCache.txt cmake_install.cmake Makefile *log.txt
./stamp.sh now.h
cd ..
tar --exclude='patcher/CMakeFiles/*' \
    --exclude='*.swp' \
    --exclude-vcs -c -v -z -f "$archive" patcher
#scp "$archive" "android:"
#scp "$archive" "android:patcher.tar.gz"
scp "$archive" "pi:build/patcher.tar.gz"
