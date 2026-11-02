#!/usr/bin/env bash
# Build Task 1 and launch QEMU using the existing LAB01/LAB02 environment.
set -euo pipefail
BASE="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TASK="$BASE/ASSIGNMENT02_TASK1_COMPLETE"
KERNEL="$BASE/LAB01/build/kernel/arch/arm/boot/zImage"
DTB="$BASE/LAB01/build/kernel/arch/arm/boot/dts/vexpress-v2p-ca9.dtb"
LABROOT="$BASE/LAB02/rootfs/initramfs"
STAGE="$TASK/output/rootfs"
ARCHIVE="$TASK/output/initramfs_asgn2.cpio.gz"
for dir in "$BASE/LAB01/build/kernel" "$LABROOT"; do
  if [[ ! -d "$dir" ]]; then echo "Missing $dir" >&2; exit 1; fi
done
for file in "$KERNEL" "$DTB"; do
  if [[ ! -f "$file" ]]; then echo "Missing $file" >&2; exit 1; fi
done
for cmd in arm-linux-gnueabihf-gcc qemu-system-arm cpio gzip; do
  command -v "$cmd" >/dev/null || { echo "Missing $cmd" >&2; exit 1; }
done
if ! grep -q '^CONFIG_POSIX_MQUEUE=y' "$BASE/LAB01/build/kernel/.config"; then
  echo "ERROR: Kernel lacks CONFIG_POSIX_MQUEUE=y. mq_open() would fail in QEMU." >&2
  echo "Enable POSIX_MQUEUE in the LAB01 ARMv7 kernel config, rebuild zImage, then retry." >&2
  exit 2
fi
printf '\n[1/4] Building ARM driver...\n'
make -C "$TASK/driver" all
printf '\n[2/4] Building ARM application...\n'
make -C "$TASK/app" arm
printf '\n[3/4] Preparing initramfs (requires sudo for /dev nodes)...\n'
mkdir -p "$STAGE" "$TASK/output"
sudo -v
# sudo is used sequentially; not twice inside the same pipeline.
sudo cp -a "$LABROOT/." "$STAGE/"
sudo mkdir -p "$STAGE/lib/modules/5.15.0" "$STAGE/usr/bin" "$STAGE/tmp"
sudo cp "$TASK/driver/sms_sensor_driver.ko" "$STAGE/lib/modules/5.15.0/"
sudo cp "$TASK/app/sms_app" "$STAGE/usr/bin/"
sudo cp "$TASK/scripts/test_task1_qemu.sh" "$STAGE/usr/bin/test_task1_qemu.sh"
sudo chmod 0755 "$STAGE/usr/bin/sms_app" "$STAGE/usr/bin/test_task1_qemu.sh"
# Only one sudo process reads the root filesystem archive.
sudo bash -c 'cd "$1" && find . -print0 | cpio --null -o -H newc' _ "$STAGE" | gzip -9 > "$ARCHIVE"
gzip -t "$ARCHIVE"
printf '\n[4/4] Launching QEMU...\n'
printf 'Inside QEMU: sh /usr/bin/test_task1_qemu.sh\n\n'
cd "$BASE"
exec qemu-system-arm -M vexpress-a9 -m 512M \
  -kernel "$KERNEL" -dtb "$DTB" -initrd "$ARCHIVE" \
  -append 'console=ttyAMA0 rdinit=/init' -nographic
