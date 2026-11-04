# Embedded Linux System

A collection of Embedded Linux labs and assignments developed on Ubuntu, targeting ARMv7 Cortex-A9 using QEMU.

## Development Environment

| Component | Technology |
|-----------|------------|
| Host OS | Ubuntu Linux |
| Target | ARMv7 Cortex-A9 |
| Kernel | Linux 5.15 |
| Emulator | QEMU vexpress-a9 |
| Toolchain | arm-linux-gnueabihf-gcc |
| Language | C |

## Labs & Assignments

| Project | Description | Status |
|---------|-------------|--------|
| [LAB01](LAB01/) | Linux Kernel, U-Boot, BusyBox and Root Filesystem | Completed |
| [LAB02](LAB02/) | Character Device Driver and File Operations | Completed |
| [LAB03](LAB03/) | GDB Debugging, Valgrind, strace, perf and cyclictest | Completed |
| [Assignment 01](ASSIGNMENT01/) | ioctl Interface and Misc Counter Driver | Completed |
| Assignment 02 | Multithreaded Sensor Monitoring System | Tasks 1–4 Completed |

## LAB03 - Debugging & Performance

LAB03 covers four main exercises:

| Part | Tool | Description |
|------|------|-------------|
| A | GDB + QEMU | Debug ARM applications and fix common bugs |
| B | Valgrind | Detect and fix memory leaks |
| C | strace / perf | Analyze system calls and CPU performance |
| D | cyclictest | Measure real-time latency |

## Assignment 02 - Sensor Monitoring

A sensor monitoring application using:

- Linux Kernel Driver
- POSIX Threads and Message Queues
- Circular Buffer with Mutex
- Temperature and Humidity Monitoring
- Logging and Alert System
- Real-Time Scheduling and Jitter Analysis

## Quick Start

Build the sensor monitoring application:

```bash
make -C ASSIGNMENT02_TASK1_COMPLETE/app host
```

Run the host tests:

```bash
bash ASSIGNMENT02_TASK1_COMPLETE/scripts/test_host.sh
```

Build and test LAB03 ARM application:

```bash
qemu-arm ./LAB03/apps/buggy_app_fixed
```

---

**Embedded Linux Systems | ARMv7 | Linux Kernel | Device Drivers**
