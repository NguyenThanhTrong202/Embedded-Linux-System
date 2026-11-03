#!/usr/bin/env python3
"""Build a short report from actual Task2 logs; never invent measurements."""
from pathlib import Path
import re

base = Path(__file__).resolve().parent.parent
logs = base / "logs"

def read(name):
    p = logs / name
    return p.read_text(errors="replace") if p.exists() else None

rows = []
for t in (5, 30, 55):
    value = read(f"memory_status_{t}s.txt")
    if not value:
        rows.append((t, "chưa đo", "chưa đo", "chưa đo"))
        continue
    def find(key):
        m = re.search(rf"^{key}:\s*(\d+)", value, re.M)
        return m.group(1) if m else "?"
    rows.append((t, find("VmRSS"), find("VmSize"), find("Threads")))

report = [
    "# Assignment 02 — Task 2: Kết quả đo thực tế",
    "",
    "Dữ liệu dưới đây lấy trực tiếp từ log kiểm thử, không phải số liệu mẫu.",
    "",
    "## Memory footprint trên QEMU",
    "",
    "| Mốc | VmRSS (kB) | VmSize (kB) | Threads |",
    "|---:|---:|---:|---:|",
]
for t, rss, vms, thr in rows:
    report.append(f"| {t}s | {rss} | {vms} | {thr} |")
valid = [int(rss) for _, rss, _, _ in rows if rss.isdigit()]
if len(valid) == 3:
    report.append(f"\nVmRSS thay đổi T=5→55s: {valid[-1] - valid[0]:+d} kB. Cần phân tích theo biến động thực tế, không coi mọi tăng RSS là memory leak.")
else:
    report.append("\nChưa đủ 3 mốc đo để kết luận xu hướng bộ nhớ.")

report += ["", "## Driver integration / ioctl"]
summary = read("qemu_task2_summary.txt")
report.append("\n```text\n" + (summary.strip() if summary else "Chưa có số liệu QEMU") + "\n```")
report += ["", "## Valgrind host x86", ""]
val = read("valgrind_sms.txt")
if val:
    found = re.findall(r"^.*(?:definitely lost:|possibly lost:|still reachable:|ERROR SUMMARY:).*$", val, re.M)
    report.append("```text\n" + "\n".join(found[-8:]) + "\n```")
else:
    report.append("Chưa có valgrind_sms.txt. Cần chạy test_host_task2.sh trên Ubuntu.")
report += [
    "", "## Ghi chú memory map", "",
    "- ARM Task1 xây dựng **static binary**. Không có thư viện `.so` ánh xạ động trong maps ARM; đây là đặc điểm build, không phải lỗi.",
    "- Host x86 là dynamic binary: dùng `host_memory_maps.txt` để giải thích các vùng libc, ld-linux, [heap], [stack], [vdso].",
    "- Với pthread, stack phụ có thể hiện dưới dạng anonymous mappings thay vì `[stack:<tid>]`.",
    "", "## Bằng chứng cần lưu", "",
    "`valgrind_sms.txt`, `host_memory_maps.txt`, `memory_status_{5,30,55}s.txt`, `memory_maps_30s.txt`, `fd_counts.txt`, `driver_ioctl_test.txt`, `proc_sms_stats_{before,after}.txt`, `sms_sample_run.txt`.",
    "", "Các file phải do kết quả chạy thực tế tạo ra trước khi đưa vào báo cáo cuối kỳ.",
]
out = logs / "analysis_task2.md"
out.write_text("\n".join(report) + "\n", encoding="utf-8")
print("Saved", out)
print("\n".join(report[:17]))
