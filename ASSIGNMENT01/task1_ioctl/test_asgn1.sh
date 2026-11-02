#!/bin/sh

set -eu

echo "===== ASSIGNMENT 01 - TASK 1 ====="

# Load driver if needed
loaded_here=0

if ! grep -q '^asgn1_driver ' /proc/modules; then
    insmod /lib/modules/5.15.0/asgn1_driver.ko
    loaded_here=1
fi

# Create device node if needed
if [ ! -c /dev/asgn1 ]; then
    mknod /dev/asgn1 c 241 0
fi

echo "===== RUNNING TESTS ====="

result=0
/usr/bin/test_asgn1 || result=$?

echo "===== KERNEL LOG ====="
dmesg | tail -40

# Unload if this script loaded the driver
if [ "$loaded_here" -eq 1 ]; then
    rmmod asgn1_driver
fi

exit "$result"
