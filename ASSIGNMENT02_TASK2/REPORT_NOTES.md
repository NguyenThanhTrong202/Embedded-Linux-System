# Gợi ý viết phần Task 2 trong báo cáo (điền số liệu thực tế)

## 1. Memory leak (x86_64)

Bằng chứng: `logs/valgrind_sms.txt` và `logs/valgrind_app_output.txt`.

- Thời gian đo: 32 giây, ứng dụng hoạt động với 4 thread và dữ liệu CSV mô phỏng.
- Phương thức dừng: SIGINT gửi đến tiến trình Valgrind/ứng dụng; xác nhận dòng `SUMMARY`.
- `definitely lost`: điền từ log thực tế.
- `possibly lost`, `still reachable`: điền từ log thực tế và giải thích nếu có.
- Khi phân tích `still reachable`, phân biệt cấp phát do glibc / POSIX message queue và bộ nhớ do ứng dụng tự cấp.

## 2. Memory footprint (ARM QEMU)

Chạy `python3 ASSIGNMENT02_TASK2/scripts/analyze_task2.py` sau khi tách log QEMU để tự lập bảng.

Mốc 5s / 30s / 55s: `VmRSS`, `VmSize`, `Threads`; nhận xét xu hướng từ các giá trị thật, không coi thay đổi RSS = leak nếu Valgrind sạch.

## 3. Phân tích /proc/PID/maps

Dựa trên `logs/memory_maps_30s.txt` (ARM) và `logs/host_memory_maps.txt` (x86):

1. **text segment**: địa chỉ có quyền `r-xp`, ánh xạ file thực thi `sms_app`.
2. **data/bss**: địa chỉ `rw-p` thuộc cùng file thực thi.
3. **heap**: vùng `[heap]`, nơi runtime quản lý malloc/calloc (nếu ứng dụng sử dụng brk heap).
4. **stack**: vùng `[stack]` là stack của main thread.
5. **vdso/vvar**: mapping nhân Linux cấp để hỗ trợ một số lời gọi thời gian/hệ thống nhanh; có thể khác nhau tùy kernel và kiến trúc.
6. **thread stack**: pthread stack có thể nằm trong anonymous `rw-p` mappings; Linux không nhất thiết hiện nhãn `[stack:tid]` trên kernel mới.
7. **shared libraries**: bản ARM đã build `-static`, vì vậy thông thường không có `libc.so` ánh xạ động; xem bản x86 để phân tích các `libc.so`, `ld-linux` mappings.

Chọn ít nhất 5 region có thực trong log của bạn và trích địa chỉ/vị trí thực tế.

## 4. Driver integration

- `logs/driver_ioctl_test.txt`: kiểm tra `SMS_SET_INTERVAL` đặt 500ms, ioctl 0ms bị từ chối, đọc CSV hợp lệ và thử đọc quá sớm.
- `logs/proc_sms_stats_before.txt` và `...after.txt`: tính chênh `read_count` trong 60s.
- `logs/fd_counts.txt`: số fd tại 5s, 30s, 55s. Đánh giá có dấu hiệu tăng liên tục không.
- `logs/sms_sample_run.txt`: có summary, thoát code 0, không crash.
- `logs/qemu_task2_summary.txt`: kết quả kiểm tra tự động. `RESULT=CHECK_LOGS` cần xem lại thay vì tự đánh dấu PASS.
