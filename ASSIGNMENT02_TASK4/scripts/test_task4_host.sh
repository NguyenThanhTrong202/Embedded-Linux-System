#!/usr/bin/env bash
set -euo pipefail
BASE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$BASE/logs"
make -C "$BASE/app" host
FIXTURE=/tmp/sms_task4.csv
printf '1000,42.5,82.0\n' > "$FIXTURE"
# This proves jitter collection on the host; the FIFO/OTHER comparison
# should be performed inside root QEMU where SCHED_FIFO can be confirmed.
"$BASE/app/sms_app_x86" -p "$FIXTURE" -i 10 -d 12 -s other > "$BASE/logs/task4_other_host.txt" 2>&1
grep -E '^JITTER|^SUMMARY' "$BASE/logs/task4_other_host.txt"
n=$(sed -n 's/^JITTER policy=.* samples=\([0-9]*\).*/\1/p' "$BASE/logs/task4_other_host.txt" | tail -1)
[[ "$n" =~ ^[0-9]+$ ]] && ((n >= 1000)) || { echo "FAIL: collected only ${n:-0} samples" >&2; exit 1; }
echo 'PASS: host jitter measurement collected >=1000 samples'
