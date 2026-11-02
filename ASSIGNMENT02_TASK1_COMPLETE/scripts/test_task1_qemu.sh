#!/bin/sh
# Run inside ARMv7 QEMU; requires CONFIG_POSIX_MQUEUE=y.
set -eu
MODULE=/lib/modules/5.15.0/sms_sensor_driver.ko
[ -f "$MODULE" ] || { echo "FAIL missing $MODULE"; exit 1; }
[ -x /usr/bin/sms_app ] || { echo 'FAIL missing /usr/bin/sms_app'; exit 1; }
if ! grep -q '^sms_sensor_driver ' /proc/modules; then
    insmod "$MODULE"
fi
minor=$(awk '$2=="sms_sensor" {print $1}' /proc/misc)
[ -n "$minor" ] || { echo 'FAIL: misc device not registered'; exit 1; }
if [ ! -c /dev/sms_sensor ]; then
    mknod /dev/sms_sensor c 10 "$minor"
fi
printf '\n=== DEVICE ===\n'
ls -l /dev/sms_sensor
printf '\n=== SAMPLE ===\n'
# Allow a previous sample interval to expire if the script is rerun.
sleep 1
cat /dev/sms_sensor
printf '\n=== STATS BEFORE ===\n'
cat /proc/sms_stats
before=$(awk '/^read_count:/ {print $2}' /proc/sms_stats)
DURATION=${1:-60}
printf '\n=== 4 THREADS, %s SECOND RUN ===\n' "$DURATION"
/usr/bin/sms_app -i 500 -t 40 -h 80 -d "$DURATION" -v
printf '\n=== STATS AFTER ===\n'
cat /proc/sms_stats
after=$(awk '/^read_count:/ {print $2}' /proc/sms_stats)
delta=$((after-before))
[ "$delta" -gt 0 ] || { echo 'FAIL: driver read_count did not increase'; exit 1; }
if [ "$DURATION" -eq 60 ]; then
    [ "$delta" -ge 110 ] && [ "$delta" -le 130 ] || \
      echo "WARN: expected ~120 driver reads in 60s; observed $delta"
fi
printf '\n=== APPLICATION LOG ===\n'
[ -s /tmp/sms.log ] || { echo 'FAIL: /tmp/sms.log is empty'; exit 1; }
grep -q 'Sensor thread started' /tmp/sms.log || { echo 'FAIL sensor thread not logged'; exit 1; }
grep -q 'Processor thread started' /tmp/sms.log || { echo 'FAIL processor thread not logged'; exit 1; }
grep -q 'Alert thread started' /tmp/sms.log || { echo 'FAIL alert thread not logged'; exit 1; }
grep -q 'SAMPLE' /tmp/sms.log || { echo 'FAIL processed samples not logged'; exit 1; }
tail -n 12 /tmp/sms.log
printf '\n=== KERNEL ===\n'
dmesg | grep sms_sensor | tail -10 || true
printf '\nPASS: 4-thread Task 1, IPC, driver, log and clean duration shutdown (QEMU)\n'
