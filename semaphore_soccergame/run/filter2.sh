#!/bin/bash
if [[ "$#" -ne 3 ]] ; then
    echo "This function allows to use the filter.sh with team properties that differ from the defaults given, for creativity purposes!"
    echo "USAGE: ./filter2.sh [TOTALENTITIES] [NUMPLAYERS] [NUMGOALIES]"
    echo "TOTALENTITIES = NUMPLAYERS + NUMGOALIES + NUMREFEREES"
    exit 1
fi

./probSemSharedMemSoccerGame | awk -f filter_log_better.awk -v totalentities=$1 -v numplayers=$2 -v numgoalies=$3 

