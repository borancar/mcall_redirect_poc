# TEE+OS PoC

Simple example of M-mode invocation and trap redirection.

## Prerequisites

- SiFive Unleashed platform
- SiFive GCC toolchain (2018.12 or later)
- SiFive OpenOCD (use 2018.12 version as later versions might have some issues
  with burning the flash).
- srecord (http://srecord.sourceforge.net/)
- GNU make (tested with version 4.2.1)

## Running

NOTE: This will invalidate the recovery flash partitions on the Unleashed
board, use at your own risk.

1. Connect the UART to the Unleashed board. UART is configured at 115200 8-N-1.
2. Set RISCV to the directory where you installed the GCC toolchain
3. Set OPENOCD to the directory where you installed OpenOCD
4. Run
    ```
    make
    make load
    ```
