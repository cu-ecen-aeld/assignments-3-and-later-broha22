#!/bin/sh

if [ $# -gt 1 ]; then
    if [ -d $1 ]; then
        matched=$(grep -r $2 $1 | wc -l)
        searched=$(find $1 -name "*" -type f | wc -l)
        echo "The number of files are $searched and the number of matching lines are $matched"
        exit 0
    else
        echo "Directory $1 does not exist"
        exit 1
    fi
else
echo "Not enough arguments specified"
exit 1
fi