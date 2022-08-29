#!/bin/bash

if [ $# -gt 1 ]; then
    echo "$2" > $1
    exit 0
else
echo "Not enough arguments specified"
exit 1
fi