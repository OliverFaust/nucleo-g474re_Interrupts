#!/bin/bash
# run.sh <elf> <tag> <stack-words-per-process> [MainApp]: 20 s board run with 3 single SWD-injected
# button interrupts (EXTI SWIER1 bit 13) at ~8, 11, 14 s; then SWD readings (stacks, TCBs, heap,
# EXTI edge config, NVIC priority of EXTI15_10).
S=${OUT:-.}; M=$(dirname $0); E=$1; T=$2; W=$3
CLI=${STM32_PROGRAMMER_CLI:-STM32_Programmer_CLI}
off=$(printf '0x%x' $((0x10 + 4 * W)))
args=(--stack "ButtonProcess=MainApp_Task(void*)::buttonProc+0x10:$((4*W))" --stack "Sender=MainApp_Task(void*)::sender+0x10:$((4*W))"
      --stack "Receiver=MainApp_Task(void*)::receiver+0x10:$((4*W))"
      --tcb "ButtonProcess=MainApp_Task(void*)::buttonProc+$off" --tcb "Sender=MainApp_Task(void*)::sender+$off" --tcb "Receiver=MainApp_Task(void*)::receiver+$off")
[ "$4" = MainApp ] && args+=(--stack MainApp=mainAppStack --stack defaultTask=defaultTaskBuffer --tcb MainApp=mainAppControlBlock --tcb defaultTask=defaultTaskControlBlock)
python3 $M/measure.py $E $S/${T}_uart.txt 20 "${args[@]}" > $S/${T}_measure.txt &
P=$!; sleep 8
for i in 1 2 3; do $CLI -c port=SWD mode=HOTPLUG -w32 0x40010410 0x00002000 > /dev/null 2>&1; sleep 3; done
wait $P
rd() { $CLI -c port=SWD mode=HOTPLUG -r32 $1 4 2>&1 | sed 's/\x1b\[[0-9;]*m//g' | grep -E '^0x' | awk '{print $3}'; }
imr=$(rd 0x40010400); rtsr=$(rd 0x40010408); ftsr=$(rd 0x4001040C); ipr=$(rd 0xE000E428)
echo "EXTI line 13: IMR1=$(( (0x$imr>>13)&1 )) RTSR1=$(( (0x$rtsr>>13)&1 )) FTSR1=$(( (0x$ftsr>>13)&1 )); NVIC EXTI15_10 priority $(( (0x$ipr & 0xff) >> 4 ))" >> $S/${T}_measure.txt
cat $S/${T}_measure.txt
