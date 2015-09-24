#!/bin/bash

# Instructions:
# to execute: sudo ./run.sh
# chmod u+x if privilege error given while attempting to execute

trap cleanup INT
trap cleanup SIGTSTP

MODULE="sysmon.ko"

function cleanup {
    rmmod $MODULE
    echo
    echo "sysmon terminated."
    gedit sysmon.backup
    exit;
}

cd /root/Design-Operating-Systems/
make -C /root/Design-Operating-Systems/
gcc to_file.c -o to_file
clear

insmod $MODULE
echo "Default uid = 1000. To change, type echo uid > /proc/sysmon_uid."
echo "Logging started. Printing log to this terminal periodically."
echo "To disable logging, type echo 0 > /proc/sysmon_toggle."
echo "Hit ctrl+c to terminate module."
echo 1000 > /proc/sysmon_uid
echo 1 > /proc/sysmon_toggle

start=$SECONDS
increment=5
while [ true ]; do
    duration=$(( SECONDS - start ))
    if [ $duration == $increment ] 
        then
            ./to_file "$( cat /proc/sysmon_log )"
            awk 'NR==1, NR==50' /proc/sysmon_log
            echo "flush" > /proc/sysmon_toggle
            increment=$(( increment + 5 ))
    fi
done
