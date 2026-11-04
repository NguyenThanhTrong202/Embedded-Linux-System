# Embedded Linux Systems

This repository contains Embedded Linux coursework for ARM Cortex-A9 using QEMU.

## Development Environment

- Host OS: Ubuntu Linux
- Target Architecture: ARMv7-A
- Target CPU: Cortex-A9
- Emulator: QEMU vexpress-a9
- Cross Compiler: arm-linux-gnueabihf-

## Projects

### LAB 01

Build an Embedded Linux system using:

- Linux Kernel 5.15
- U-Boot 2022.04
- BusyBox 1.35.0
- Initramfs
- QEMU

### LAB 02

Linux Device Driver and File Systems:

- Character Device Driver
- procfs and sysfs
- MTD and JFFS2
- BusyBox Init

### LAB 03

Debugging and Performance Analysis:

- GDB Remote Debugging with QEMU
- Memory Leak Detection using Valgrind
- System Call Analysis using strace
- CPU Profiling using perf
- Real-Time Latency Measurement using cyclictest

### Assignment 01

How to Write a Device Driver:

- ioctl Interface
- Misc Counter Device

### Assignment 02

Sensor Monitoring System:

- Kernel Sensor Driver
- POSIX Threads and Message Queues
- Circular Buffer and Thread Synchronization
- Sensor Data Logging and Alert System
- Real-Time Scheduling and Jitter Analysis
