#!/bin/bash

if [ $# -ne 2 ]; then
    echo "ERROR: This script requires two arguments:
                 $0 FILE STRING

        FILE    path to a file on the filesystem
        STRING  text string which will be searched within $FILES_DIR        
"
    exit 1
fi

writefile=$1
writestr=$2

dirname="$(dirname $writefile)"
mkdir -p $dirname

echo "$writestr" > $writefile

if [ $? != 0 ]; then
    echo "The file $writefile could not be created"
    exit 1
fi

