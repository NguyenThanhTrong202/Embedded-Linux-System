#!/bin/sh
# Run inside ARMv7 QEMU after build_and_qemu.sh.
set -eu
if ! grep -q '^sms_sensor_driver ' /proc/modules; then
    insmod /lib/modules/5.15.0/sms_sensor_driver.ko
fi
minor=$(awk '$2=="sms_sensor" {print $1}' /proc/misc)
[ -n "$minor" ] || { echo 'ERROR: driver not registered'; exit 1; }
[ -c /dev/sms_sensor ] || mknod /dev/sms_sensor c 10 "$minor"
mkdir -p /tmp
# 20ms x 30s => approximately 1500 wake-ups for each scheduling policy.
/usr/bin/sms_app -i 20 -d 30 -s other > /tmp/task4_other.txt 2>&1
/usr/bin/sms_app -i 20 -d 30 -s fifo > /tmp/task4_fifo.txt 2>&1
printf '\n=== SCHED_OTHER ===\n'
grep -E 'SCHED_OTHER|^JITTER|^SUMMARY' /tmp/task4_other.txt || true
printf '\n=== SCHED_FIFO ===\n'
grep -E 'SCHED_FIFO|^JITTER|^SUMMARY' /tmp/task4_fifo.txt || true
for mode in other fifo; do
    n=$(sed -n 's/^JITTER policy=.* samples=\([0-9]*\).*/\1/p' "/tmp/task4_${mode}.txt" | tail -1)
    [ -n "$n" ] && [ "$n" -ge 1000 ] || { echo "FAIL: $mode jitter sample count <$n>"; exit 1; }
done
if ! grep -q 'SCHED_FIFO priority 50 enabled' /tmp/task4_fifo.txt; then
    echo 'WARNING: SCHED_FIFO priority 50 was not granted; comparison is invalid.'
    exit 1
fi
echo 'PASS: both scheduling policies collected >= 1000 jitter samples'
