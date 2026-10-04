#!/bin/bash
# burst_read.sh <elf> <tag>: flash + 10 s UART log, then read the burst-test counters over SWD
S=${OUT:-.}; E=$1; T=$2; CLI=${STM32_PROGRAMMER_CLI:-STM32_Programmer_CLI}
python3 $NUCLEO_RUN $E $S/${T}_uart.txt 10 | tail -1
rd() { a=$(arm-none-eabi-nm $E | awk -v s=$1 '$3==s{print $1}'); $CLI -c port=SWD mode=HOTPLUG -r32 0x$a $2 2>&1 | sed 's/\x1b\[[0-9;]*m//g' | grep -E '^0x' | cut -d: -f2 | tr -s ' ' '\n' | grep -v '^$' | while read v; do printf '%d ' 0x$v; done; }
n=$(rd t_deliv 4)
echo "burst done=$(rd t_burst_done 4)| interrupts=$(rd t_isr 4)| putFromISR failures=$(rd t_fail 4)| events delivered=$n| delivered seq: $(rd t_seq $((4 * (n < 32 ? n : 32))))"
