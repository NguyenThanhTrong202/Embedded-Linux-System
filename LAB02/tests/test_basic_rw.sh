#!/bin/sh

set -eu

DEVICE="/dev/lab2"

echo "=== LAB02 Character Device Test ==="

if [ ! -c "$DEVICE" ]; then
    echo "FAIL: Device not found"
    exit 1
fi

echo "[TEST 1] Write HELLO"
printf "HELLO" > "$DEVICE"

echo "[TEST 2] Read HELLO"
RESULT=$(cat "$DEVICE")

if [ "$RESULT" = "HELLO" ]; then
    echo "PASS: Read/Write"
else
    echo "FAIL: Read/Write"
    exit 1
fi

echo "[TEST 3] Overwrite buffer"
printf "WORLD" > "$DEVICE"

RESULT=$(cat "$DEVICE")

if [ "$RESULT" = "WORLD" ]; then
    echo "PASS: Buffer overwrite"
else
    echo "FAIL: Buffer overwrite"
    exit 1
fi

echo "=== ALL TESTS PASSED ==="
