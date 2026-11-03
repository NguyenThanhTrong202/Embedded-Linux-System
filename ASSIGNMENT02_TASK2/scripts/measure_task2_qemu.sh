#!/bin/sh
# Run INSIDE QEMU after boot. Capture real /proc measurements for Task 2.
set -eu
MODULE=/lib/modules/5.15.0/sms_sensor_driver.ko
APP=/usr/bin/sms_app
CHECK=/usr/bin/test_sms_ioctl
LOGS=/tmp/task2_logs
mkdir -p "$LOGS"

publish() {
    key=$1
    path=$2
    printf '\n<<<TASK2:BEGIN:%s>>>\n' "$key"
    if [ -f "$path" ]; then cat "$path"; else echo "MISSING: $path"; fi
    printf '\n<<<TASK2:END:%s>>>\n' "$key"
}

if [ ! -e /proc/self/status ]; then mount -t proc proc /proc; fi
if ! grep -q '^sms_sensor_driver ' /proc/modules; then
    insmod "$MODULE"
fi
minor=$(awk '$2=="sms_sensor" {print $1}' /proc/misc)
[ -n "$minor" ] || { echo 'ERROR: sensor misc device not registered'; exit 1; }
[ -c /dev/sms_sensor ] || mknod /dev/sms_sensor c 10 "$minor"

printf '\n[Task2 1/4] ioctl/CSV/procfs tests...\n'
"$CHECK" > "$LOGS/driver_ioctl_test.txt" 2>&1 || {
    cat "$LOGS/driver_ioctl_test.txt"
    echo 'FAIL: ioctl integration test'
    exit 1
}
cat "$LOGS/driver_ioctl_test.txt"
# Capture before after the test harness, since the harness intentionally reads once.
cat /proc/sms_stats > "$LOGS/proc_sms_stats_before.txt"
before=$(awk '/^read_count:/ {print $2}' "$LOGS/proc_sms_stats_before.txt")

printf '\n[Task2 2/4] Run 60s, inspect process at T=5/30/55...\n'
"$APP" -i 500 -t 40 -h 80 -d 60 > "$LOGS/sms_sample_run.txt" 2>&1 &
pid=$!

snapshot() {
    label=$1
    sleep "$2"
    if [ ! -r "/proc/$pid/status" ]; then
        echo "FAIL: sms_app was not running at T=$label" >&2
        return 1
    fi
    {
        echo "T=$label"
        echo "PID=$pid"
        grep -E '^(Name|State|Threads|VmSize|VmRSS|VmPeak|VmHWM):' "/proc/$pid/status" || true
    } > "$LOGS/memory_status_${label}.txt"
    count=$(ls -1 "/proc/$pid/fd" 2>/dev/null | wc -l | tr -d ' ')
    echo "T=$label fd_count=$count" >> "$LOGS/fd_counts.txt"
    echo "[$label] $(grep -E '^(VmRSS|VmSize|Threads):' "$LOGS/memory_status_${label}.txt" | tr '\n' ';')"
    if [ "$label" = '30s' ]; then
        cat "/proc/$pid/maps" > "$LOGS/memory_maps_30s.txt"
        {
            echo '=== /proc/<pid>/task threads ==='
            ls -1 "/proc/$pid/task"
            echo '=== /proc/<pid>/maps mappings containing stack, heap, vdso ==='
            grep -E '\[heap\]|\[stack\]|\[vdso\]|\[vvar\]' "/proc/$pid/maps" || true
            echo 'ARM app is statically linked: no shared libc.so mapping is expected.'
        } > "$LOGS/memory_regions_notes.txt"
    fi
}

: > "$LOGS/fd_counts.txt"
snapshot 5s 5
snapshot 30s 25
snapshot 55s 25

printf '\n[Task2 3/4] Waiting for normal shutdown...\n'
set +e
wait "$pid"
rc=$?
set -e
cat /proc/sms_stats > "$LOGS/proc_sms_stats_after.txt"
after=$(awk '/^read_count:/ {print $2}' "$LOGS/proc_sms_stats_after.txt")
delta=$((after - before))
[ -f /tmp/sms.log ] && cp /tmp/sms.log "$LOGS/sms.log" || true
dmesg | grep 'sms_sensor' > "$LOGS/dmesg_sms.txt" || true
{
    echo "APP_EXIT_CODE=$rc"
    echo "DRIVER_READS_BEFORE=$before"
    echo "DRIVER_READS_AFTER=$after"
    echo "DRIVER_READS_DELTA=$delta"
    echo 'EXPECTED_AROUND_120_60_SECONDS_INTERVAL_500MS=1'
    if [ "$rc" -eq 0 ] && [ "$delta" -ge 110 ] && [ "$delta" -le 130 ] &&
        [ -s "$LOGS/sms.log" ] &&
        grep -q '^SUMMARY ' "$LOGS/sms_sample_run.txt"; then
        echo 'RESULT=PASS'
    else
        echo 'RESULT=CHECK_LOGS'
    fi
} > "$LOGS/qemu_task2_summary.txt"

printf '\n[Task2 4/4] Exporting log blocks to host console...\n'
for name in \
    driver_ioctl_test \
    proc_sms_stats_before proc_sms_stats_after \
    memory_status_5s memory_status_30s memory_status_55s \
    memory_maps_30s memory_regions_notes fd_counts \
    sms_sample_run sms dmesg_sms qemu_task2_summary; do
    case "$name" in
        sms) publish 'sms.log' "$LOGS/sms.log" ;;
        *) publish "$name.txt" "$LOGS/$name.txt" ;;
    esac
done
printf '\nTask2 measurements completed. Check RESULT in qemu_task2_summary.\n'
printf 'Exit QEMU with Ctrl+A then X; on host run extract_qemu_logs.py.\n'
