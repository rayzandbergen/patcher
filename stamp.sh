#!/bin/bash
echo "#define NOW \""`date '+%d %b %Y'`"\"" > "$1"
echo "#define SVN \""`svnversion -n`"\"" >> "$1"

