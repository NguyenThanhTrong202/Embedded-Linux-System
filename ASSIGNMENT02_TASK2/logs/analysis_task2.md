# Assignment 02 — Task 2: Kết quả đo thực tế

Dữ liệu dưới đây lấy trực tiếp từ log kiểm thử, không phải số liệu mẫu.

## Memory footprint trên QEMU

| Mốc | VmRSS (kB) | VmSize (kB) | Threads |
|---:|---:|---:|---:|
| 5s | 4 | 34568 | 5 |
| 30s | 4 | 34568 | 5 |
| 55s | 4 | 34568 | 5 |

VmRSS thay đổi T=5→55s: +0 kB. Cần phân tích theo biến động thực tế, không coi mọi tăng RSS là memory leak.

## Driver integration / ioctl

```text
APP_EXIT_CODE=0
DRIVER_READS_BEFORE=1
DRIVER_READS_AFTER=118
DRIVER_READS_DELTA=117
EXPECTED_AROUND_120_60_SECONDS_INTERVAL_500MS=1
RESULT=PASS
```

## Valgrind host x86

```text
==7926== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

## Ghi chú memory map

- ARM Task1 xây dựng **static binary**. Không có thư viện `.so` ánh xạ động trong maps ARM; đây là đặc điểm build, không phải lỗi.
- Host x86 là dynamic binary: dùng `host_memory_maps.txt` để giải thích các vùng libc, ld-linux, [heap], [stack], [vdso].
- Với pthread, stack phụ có thể hiện dưới dạng anonymous mappings thay vì `[stack:<tid>]`.

## Bằng chứng cần lưu

`valgrind_sms.txt`, `host_memory_maps.txt`, `memory_status_{5,30,55}s.txt`, `memory_maps_30s.txt`, `fd_counts.txt`, `driver_ioctl_test.txt`, `proc_sms_stats_{before,after}.txt`, `sms_sample_run.txt`.

Các file phải do kết quả chạy thực tế tạo ra trước khi đưa vào báo cáo cuối kỳ.
