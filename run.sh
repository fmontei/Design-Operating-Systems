#!/bin/bash

# Instructions:
# to execute: sudo ./run.sh
# chmod u+x if privilege error given while attempting to execute

trap cleanup INT
trap cleanup SIGTSTP

MODULE="sysmon.ko"
TEXT_FILE="sysmon.backup"

# Unregister module and launch text file with log contents
function cleanup {
    rmmod $MODULE
    echo
    echo "sysmon terminated."
    gedit $TEXT_FILE
    exit;
}

# Query user if they want to delete the pre-existing text file if it exists
if [ -f $TEXT_FILE ]
    then
        while [ true ]; do
            read -p "Text file already exists. Do you wish to clear it? (y/n) " yn
            case $yn in
                [Yy]* ) rm $TEXT_FILE; break;;
                [Nn]* ) break;;
                * ) echo "Please answer yes or no.";;
            esac
        done
fi

while [ true ]; do
    read -p "UID to monitor: " uid
    case $uid in
        [0-9]* ) break;;
        * ) echo "UID must be a number.";;
    esac
done

cd /root/Design-Operating-Systems/
make -C /root/Design-Operating-Systems/
gcc to_file.c -o to_file
clear

insmod $MODULE
echo ">>>>> Default uid = 1000. To change, type echo uid > /proc/sysmon_uid."
echo ">>>>> Logging started. Printing log to this terminal periodically."
echo ">>>>> To disable logging, type echo 0 > /proc/sysmon_toggle."
echo ">>>>> Hit ctrl+c to terminate module."
echo $uid > /proc/sysmon_uid
echo 1 > /proc/sysmon_toggle

start=$SECONDS
increment=5
while [ true ]; do
    duration=$(( SECONDS - start ))
    if [ $duration == $increment ] 
        then
            ./to_file "$( cat /proc/sysmon_log )" # Print entire log to file
            echo ">>>>> Printing first 10 lines:"
            awk 'NR==1, NR==10' /proc/sysmon_log # Print first 10 lines of log
            echo "flush" > /proc/sysmon_toggle # Tell module to clear log
            increment=$(( increment + 1 ))
    fi
done
