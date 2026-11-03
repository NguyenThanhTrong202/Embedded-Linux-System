#!/usr/bin/env bash
# Independent QEMU image for ASM2 Task2, based on already working Task1.
set -euo pipefail
BASE="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TASK1="$BASE/ASSIGNMENT02_TASK1_COMPLETE"
TASK2="$BASE/ASSIGNMENT02_TASK2"
KERNEL="$BASE/LAB01/build/kernel/arch/arm/boot/zImage"
DTB="$BASE/LAB01/build/kernel/arch/arm/boot/dts/vexpress-v2p-ca9.dtb"
LABROOT="$BASE/LAB02/rootfs/initramfs"
STAGE="$TASK2/output/rootfs"
ARCHIVE="$TASK2/output/initramfs_asgn2_task2.cpio.gz"
LOGS="$TASK2/logs"
for file in "$KERNEL" "$DTB" "$TASK1/app/sms_app.c" "$TASK1/driver/sms_sensor_driver.c"; do
    if [[ ! -f "$file" ]]; then echo "ERROR: missing $file" >&2; exit 1; fi
done
[[ -d "$LABROOT" ]] || { echo "ERROR: missing LAB02 rootfs $LABROOT" >&2; exit 1; }
for cmd in arm-linux-gnueabihf-gcc qemu-system-arm cpio gzip tee; do
    command -v "$cmd" >/dev/null || { echo "Missing $cmd" >&2; exit 1; }
done
if ! grep -q '^CONFIG_POSIX_MQUEUE=y' "$BASE/LAB01/build/kernel/.config"; then
    echo 'ERROR: CONFIG_POSIX_MQUEUE=y is not enabled in LAB01 kernel.' >&2
    exit 1
fi
printf '\n[1/4] Build Task1 unchanged + ioctl test...\n'
make -C "$TASK1/driver"
make -C "$TASK1/app" arm
make -C "$TASK2/tests" arm
file "$TASK1/driver/sms_sensor_driver.ko" "$TASK1/app/sms_app" "$TASK2/tests/test_sms_ioctl"

printf '\n[2/4] Create new independent Task2 initramfs...\n'
mkdir -p "$STAGE" "$LOGS"
sudo -v
sudo cp -a "$LABROOT/." "$STAGE/"
sudo mkdir -p "$STAGE/lib/modules/5.15.0" "$STAGE/usr/bin" "$STAGE/tmp"
sudo cp "$TASK1/driver/sms_sensor_driver.ko" "$STAGE/lib/modules/5.15.0/"
sudo cp "$TASK1/app/sms_app" "$STAGE/usr/bin/"
sudo cp "$TASK2/tests/test_sms_ioctl" "$STAGE/usr/bin/"
sudo cp "$TASK2/scripts/measure_task2_qemu.sh" "$STAGE/usr/bin/"
sudo chmod 0755 "$STAGE/usr/bin/sms_app" "$STAGE/usr/bin/test_sms_ioctl" "$STAGE/usr/bin/measure_task2_qemu.sh"

printf '\n[3/4] Pack and verify cpio.gz...\n'
sudo bash -c 'cd "$1" && find . -print0 | cpio --null -o -H newc' _ "$STAGE" | gzip -9 > "$ARCHIVE"
gzip -t "$ARCHIVE"

printf '\n[4/4] Boot QEMU (console captured at logs/qemu_console_capture.txt)...\n'
printf 'Inside QEMU: sh /usr/bin/measure_task2_qemu.sh\n'
printf 'To exit QEMU: Ctrl+A, then X\n'
printf 'Back on Ubuntu: python3 ASSIGNMENT02_TASK2/scripts/extract_qemu_logs.py\n\n'
cd "$BASE"
exec qemu-system-arm -M vexpress-a9 -m 512M \
    -kernel "$KERNEL" -dtb "$DTB" -initrd "$ARCHIVE" \
    -append 'console=ttyAMA0 rdinit=/init' -nographic \
    > >(tee "$LOGS/qemu_console_capture.txt") 2>&1
