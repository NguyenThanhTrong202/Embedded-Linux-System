#!/usr/bin/env python3
"""Extract guest log sections from host-captured QEMU console, without 9p/network."""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parent.parent
logs = root / "logs"
source = logs / "qemu_console_capture.txt"
if not source.is_file():
    sys.exit(f"ERROR: {source} missing; run build_and_run_qemu.sh first")
contents = source.read_bytes().decode("utf-8", errors="replace").replace("\r", "")
pattern = re.compile(r"<<<TASK2:BEGIN:([\w.]+)>>>(.*?)<<<TASK2:END:\1>>>", re.S)
blocks = pattern.findall(contents)
if not blocks:
    sys.exit("ERROR: No complete TASK2 blocks. Run measure_task2_qemu.sh inside QEMU, wait until finished, then exit.")
allowed = {
    "driver_ioctl_test.txt", "proc_sms_stats_before.txt", "proc_sms_stats_after.txt",
    "memory_status_5s.txt", "memory_status_30s.txt", "memory_status_55s.txt",
    "memory_maps_30s.txt", "memory_regions_notes.txt", "fd_counts.txt",
    "sms_sample_run.txt", "sms.log", "dmesg_sms.txt", "qemu_task2_summary.txt",
}
for name, data in blocks:
    if name not in allowed:
        print(f"SKIP unexpected filename: {name}")
        continue
    path = logs / name
    path.write_text(data.strip("\n") + "\n", encoding="utf-8")
    print(f"SAVED {path.relative_to(root)} ({path.stat().st_size} bytes)")
print(f"Extracted {len(blocks)} blocks")
summary = logs / "qemu_task2_summary.txt"
if summary.exists():
    print("\n--- REAL QEMU SUMMARY ---")
    print(summary.read_text())
