#!/bin/sh

$DEVKITARM/bin/arm-none-eabi-gdb 3ds/out/output.elf -ex "target remote :24689"
