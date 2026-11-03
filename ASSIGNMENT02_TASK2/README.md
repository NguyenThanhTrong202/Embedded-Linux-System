# ASSIGNMENT 02 — Task 2: Memory & Driver Integration

Bộ công cụ **không ghi đè** code Task 1 đã PASS trong `ASSIGNMENT02_TASK1_COMPLETE/`.

## 1) Thư mục

Giải nén ZIP tại `~/Embedded-Linux-System`:

```bash
unzip ~/Downloads/ASSIGNMENT02_Task2_Toolkit.zip -d ~/Embedded-Linux-System
cd ~/Embedded-Linux-System
```

Cấu trúc: `ASSIGNMENT02_TASK2/{scripts,tests,logs,output}`. Lưu ý file `logs/` chỉ được tạo **sau khi kiểm thử**, không có log mẫu giả.

## 2) Trên Ubuntu — kiểm tra Valgrind >=30s, SIGINT và host memory maps

Nếu chưa có công cụ: `sudo apt update && sudo apt install -y valgrind`.

```bash
bash ASSIGNMENT02_TASK2/scripts/test_host_task2.sh
```

Log thật sẽ nằm tại:
- `ASSIGNMENT02_TASK2/logs/valgrind_sms.txt` (output Valgrind đầy đủ)
- `ASSIGNMENT02_TASK2/logs/host_memory_maps.txt` (ELF động có shared libraries)
- `ASSIGNMENT02_TASK2/logs/host_memory_status.txt`
- `ASSIGNMENT02_TASK2/logs/host_app_output.txt`
- `ASSIGNMENT02_TASK2/logs/host_summary.txt`

**Không tự nhận PASS khi `definitely lost` khác 0 hoặc tiến trình không dừng sạch.**

## 3) Trên Ubuntu — chạy QEMU và thu log

```bash
bash ASSIGNMENT02_TASK2/scripts/build_and_run_qemu.sh
```

Trong QEMU (sau khi vào shell `/ #`):

```sh
sh /usr/bin/measure_task2_qemu.sh
```

Script này sẽ chạy ứng dụng 60s, đo bộ nhớ tại T=5,30,55s; lưu `/proc/PID/maps`, fd count, `/proc/sms_stats` trước/sau; kiểm tra ioctl thực tế và in dữ liệu ra console theo khối `<<<TASK2:BEGIN:...>>>`.

Kết thúc QEMU bằng **Ctrl+A rồi X**. Trên Ubuntu chạy:

```bash
python3 ASSIGNMENT02_TASK2/scripts/extract_qemu_logs.py
```

Các file ở `ASSIGNMENT02_TASK2/logs/`: `qemu_console_capture.txt`, `qemu_task2_summary.txt`, `proc_sms_stats_before.txt`, `proc_sms_stats_after.txt`, `memory_status_5s.txt`, `memory_status_30s.txt`, `memory_status_55s.txt`, `memory_maps_30s.txt`, `fd_counts.txt`, `sms_sample_run.txt`, `sms.log`, `driver_ioctl_test.txt`, `dmesg_sms.txt`, v.v.

## 4) Điều kiện kiểm thử Task 2

- Valgrind `--leak-check=full` hoạt động >=30 giây, shutdown bằng SIGINT, `definitely lost: 0 bytes in 0 blocks`.
- QEMU 60 giây, read_count trong `/proc/sms_stats` tăng xấp xỉ 120 (110 đến 130 là ngưỡng tham khảo, tùy tải QEMU).
- VmRSS / VmSize đo tại 5, 30, 55 giây; tránh kết luận RSS ổn định nếu chưa so sánh số liệu thật.
- `test_sms_ioctl` xác nhận ioctl 500ms, từ chối 0ms, đọc CSV, EAGAIN khi đọc quá sớm.
- `fd_counts.txt` để phân tích file descriptors; đếm lúc chạy chứ không nhầm với leak sau exit.
- `memory_maps_30s.txt` là ARM **static executable** từ Task 1: không có `libc.so` ánh xạ động. `host_memory_maps.txt` từ x86 dynamic executable bổ sung phần shared libraries. Thông tin thread stack trên Linux hiện đại có thể nằm trong anonymous mmap, không nhất thiết ghi `[stack:<tid>]`.

Không chỉnh sửa/sao chép log mẫu vào bài nộp; các log này phải là output thực tế từ máy của bạn.
