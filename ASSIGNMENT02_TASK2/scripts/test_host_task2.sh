#!/usr/bin/env bash
# Host side memory map and Valgrind (>=30 seconds, SIGINT shutdown).
set -euo pipefail
BASE="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TASK1="$BASE/ASSIGNMENT02_TASK1_COMPLETE"
TASK2="$BASE/ASSIGNMENT02_TASK2"
LOGS="$TASK2/logs"
mkdir -p "$LOGS"
if [[ ! -f "$TASK1/app/sms_app.c" ]]; then
    echo "ERROR: missing $TASK1/app/sms_app.c; extract Task1 first." >&2
    exit 1
fi
if ! command -v valgrind >/dev/null 2>&1; then
    echo 'ERROR: Valgrind missing. Install on Ubuntu with: sudo apt update && sudo apt install -y valgrind' >&2
    exit 2
fi
make -C "$TASK1/app" host
APP="$TASK1/app/sms_app_x86"
MOCK="$(mktemp /tmp/sms_sensor_mock.XXXXXX)"
printf '1000,25.0,55.0\n' > "$MOCK"
vgpid=""
app_pid=""
cleanup() {
    if [[ -n "$app_pid" ]] && kill -0 "$app_pid" 2>/dev/null; then
        kill -INT "$app_pid" 2>/dev/null || true
        wait "$app_pid" 2>/dev/null || true
    fi
    if [[ -n "$vgpid" ]] && kill -0 "$vgpid" 2>/dev/null; then
        kill -INT "$vgpid" 2>/dev/null || true
        wait "$vgpid" 2>/dev/null || true
    fi
    rm -f "$MOCK"
}
trap cleanup EXIT
printf '\n[1/3] Capturing dynamic x86 maps, VmRSS, VmSize, Threads...\n'
"$APP" -p "$MOCK" -i 500 -d 12 > "$LOGS/host_app_output.txt" 2>&1 &
app_pid=$!
sleep 5
if [[ ! -r "/proc/$app_pid/maps" ]]; then
    echo 'FAIL: host sms_app ended before maps capture' >&2
    wait "$app_pid" || true
    exit 1
fi
cp "/proc/$app_pid/maps" "$LOGS/host_memory_maps.txt"
grep -E '^(Name|State|Threads|VmSize|VmRSS|VmPeak|VmHWM):' "/proc/$app_pid/status" > "$LOGS/host_memory_status.txt" || true
printf 'Host pid=%s, fd_count=%s\n' "$app_pid" "$(find "/proc/$app_pid/fd" -mindepth 1 -maxdepth 1 | wc -l)" >> "$LOGS/host_memory_status.txt"
wait "$app_pid"
app_pid=""

printf '\n[2/3] Valgrind >=30s + SIGINT shutdown...\n'
# Valgrind writes a complete report after the guest process exits.
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
    --track-fds=yes --error-exitcode=21 \
    --log-file="$LOGS/valgrind_sms.txt" \
    "$APP" -p "$MOCK" -i 500 -d 0 \
    > "$LOGS/valgrind_app_output.txt" 2>&1 &
vgpid=$!
sleep 32
if ! kill -0 "$vgpid" 2>/dev/null; then
    echo 'FAIL: Valgrind process exited before the 30-second run finished.' >&2
    wait "$vgpid" || true
    exit 1
fi
kill -INT "$vgpid"
set +e
wait "$vgpid"
rc=$?
set -e
vgpid=""

printf '\n[3/3] Checking Valgrind and shutdown results...\n'
{
    echo "Valgrind exit code: $rc"
    echo '--- leaked bytes ---'
    grep -E 'definitely lost:|indirectly lost:|possibly lost:|still reachable:|ERROR SUMMARY:' "$LOGS/valgrind_sms.txt" || true
    echo '--- app shutdown ---'
    grep -E '^SUMMARY ' "$LOGS/valgrind_app_output.txt" || true
} | tee "$LOGS/host_summary.txt"
if [[ "$rc" -ne 0 ]]; then
    echo "FAIL: Valgrind/app returned $rc; see logs/valgrind_sms.txt" >&2
    exit 1
fi
if ! grep -Eq 'definitely lost: +0 bytes in 0 blocks|All heap blocks were freed -- no leaks are possible' "$LOGS/valgrind_sms.txt"; then
    echo 'FAIL: no-leak conclusion not proven; inspect valgrind_sms.txt' >&2
    exit 1
fi
if ! grep -q '^SUMMARY ' "$LOGS/valgrind_app_output.txt"; then
    echo 'FAIL: graceful shutdown summary not found' >&2
    exit 1
fi
printf '\nPASS: Valgrind 32s, definitely lost=0, SIGINT shutdown, host maps saved\n'
