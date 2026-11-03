ASM2 TASK 4 - REAL-TIME JITTER ANALYSIS

Task 1 code is preserved in ASSIGNMENT02_TASK1_COMPLETE.

Host smoke check:
  bash ASSIGNMENT02_TASK4/scripts/test_task4_host.sh

QEMU ARMv7 build/boot (requires LAB01 kernel and LAB02 rootfs):
  bash ASSIGNMENT02_TASK4/scripts/build_and_qemu.sh
In QEMU (/ #):
  sh /usr/bin/test_task4_qemu.sh

The QEMU test runs OTHER and FIFO for 30s each at a 20ms sampling period;
change -i and -d for an experiment with 500ms intervals. At 500ms,
1000 jitter observations take at least 500s per policy.

Important: Verify the printed SCHED_FIFO priority 50 ENABLED line.
If FIFO scheduling is denied, do not claim the comparison is valid.
If mlockall prints FAILED, document the limit rather than claiming
memory locking is enabled. Timing on QEMU is not representative of hardware RT.
Data is printed as JITTER and JITTER_HIST in terminal; capture authentic logs.
