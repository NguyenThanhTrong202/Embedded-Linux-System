#!/usr/bin/env bash
# Test 4 threads using a regular CSV fixture. Actual kernel integration is tested in QEMU.
set -euo pipefail
TASK="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
make -C "$TASK/app" host
fixture=$(mktemp)
log=$(mktemp)
trap 'rm -f "$fixture" "$log"' EXIT
printf '1500,42.5,82.0\n' > "$fixture"
"$TASK/app/sms_app_x86" -p "$fixture" -d 2 -i 100 -v >"$log" 2>&1
for pattern in 'SMS started:' '\[SENSOR\]' '\[ALERT\]' 'SUMMARY samples='; do
    if ! grep -q "$pattern" "$log"; then
        echo "FAIL: missing $pattern" >&2
        cat "$log" >&2
        exit 1
    fi
done
if ! grep -q 'Sensor thread started' /tmp/sms.log || \
   ! grep -q 'Processor thread started' /tmp/sms.log || \
   ! grep -q 'Alert thread started' /tmp/sms.log; then
    echo 'FAIL: missing thread logging' >&2
    exit 1
fi
if "$TASK/app/sms_app_x86" -p "$fixture" -i 0 -d 1 >/dev/null 2>&1; then
    echo 'FAIL: invalid argument accepted' >&2
    exit 1
fi
"$TASK/app/sms_app_x86" -p "$fixture" -i 100 -d 0 >"$log" 2>&1 &
pid=$!
sleep 1
kill -INT "$pid"
wait "$pid"
grep -q 'SUMMARY samples=' "$log"
echo 'PASS: 4-thread host smoke, logging, alerts, invalid args, Ctrl+C shutdown'
