# LAB02 - Character Device Testing

## Environment

- Ubuntu Linux Host
- Linux Kernel 5.15
- ARMv7
- QEMU vexpress-a9
- BusyBox Root Filesystem

## Driver Information

Driver name: lab2_driver

Device: /dev/lab2

Major number: 240

## Test 1 - Load Module

insmod /lib/modules/5.15.0/lab2_driver.ko

Check:

lsmod
dmesg | tail

## Test 2 - Write Data

echo "HELLO" > /dev/lab2

## Test 3 - Read Data

cat /dev/lab2

## Test 4 - Overwrite Buffer

echo "WORLD" > /dev/lab2

cat /dev/lab2

## Test 5 - Unload Module

rmmod lab2_driver

## Automated Test

Run inside QEMU after copying the test script:

sh /path/to/test_basic_rw.sh

## Expected Result

Driver loads successfully.

Read and write operations work.

Buffer overwrite works.

Driver unloads successfully.
