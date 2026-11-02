# ARMv7 QEMU verification

The driver `driver/sms_sensor_driver.c` here is copied from the corrected `ASSIGNMENT02_PART1` revision that produced successful ARMv7/QEMU logs, including the `div_u64()` fix. The 4-thread application is a newly integrated step and needs verification in the user's QEMU guest.

Host tests use a normal CSV file and cannot replace driver/QEMU testing. `scripts/build_and_qemu.sh` refuses to start the guest if `CONFIG_POSIX_MQUEUE=y` is missing from `LAB01/build/kernel/.config`.

No fake QEMU output is supplied; capture real terminal and log output for the final report.
