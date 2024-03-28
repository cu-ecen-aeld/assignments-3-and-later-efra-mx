#!/bin/sh

if [ $# -ne 2 ]; then
    echo "ERROR: This script requires two arguments:
                 $0 FILES_DIR SEARCH_STR

        FILES_DIR   path to a directory on the filesystem
        SEARCH_STR  text string which will be searched within $FILES_DIR        
"
    exit 1
fi

filesdir=$1
searchstr=$2

if [ ! -d  "$filesdir" ]; then
    echo "Directory $filesdir does not exist"
    exit 1
fi

cd $filesdir
items_count=$(find . -type f | grep -c ^)
matching_lines=$(find . -type f -exec grep "$searchstr" {} \; | grep -c ^)
echo "The number of files are $items_count and the number of matching lines are $matching_lines"

