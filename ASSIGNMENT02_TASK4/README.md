# ASSIGNMENT02 – Task 1 (4 luồng)

**Bản hoàn chỉnh của Task 1** để kiểm tra trên Ubuntu / QEMU ARMv7.

Được tạo tách riêng, không thay thế `ASSIGNMENT02` hoặc `ASSIGNMENT02_PART1`.

## Các thành phần

- `driver/`: `sms_sensor_driver.ko`, CSV nhiệt độ/độ ẩm, `/proc/sms_stats`, `SMS_SET_INTERVAL`.
- `app/`: Sensor, Processor, Alert, Logger Threads; circular buffer + POSIX mqueue.
- `scripts/test_host.sh`: smoke test với CSV fixture trên Ubuntu.
- `scripts/build_and_qemu.sh`: build ARM và boot QEMU.
- `scripts/test_task1_qemu.sh`: chạy trong QEMU để kiểm thử 60 giây.

## Chạy

Tại `~/Embedded-Linux-System`:

```bash
bash ASSIGNMENT02_TASK1_COMPLETE/scripts/test_host.sh
bash ASSIGNMENT02_TASK1_COMPLETE/scripts/build_and_qemu.sh
```

Trong QEMU:

```sh
sh /usr/bin/test_task1_qemu.sh
```

Ứng dụng mặc định chạy 60 giây, hỗ trợ `-i`, `-t`, `-h`, `-d`, `-v`, `-p`.
Ví dụ: `sms_app -i 500 -t 40 -h 80 -d 60 -v`.

**Lưu ý:** POSIX MQ đòi hỏi `CONFIG_POSIX_MQUEUE=y` trong Kernel. Nếu chưa bật, script host sẽ báo lỗi để bạn cấu hình lại.

**Chưa phải kết quả kiểm thử QEMU thực tế:** chỉ coi là PASS sau khi chạy trên máy Ubuntu của bạn.

Đề ASM2 yêu cầu mã nguồn do sinh viên tự viết và hiểu, không nộp nguyên code tham khảo do AI sinh ra. Hãy kiểm tra/chỉnh sửa và ghi rõ việc tham khảo theo quy định của môn học.
