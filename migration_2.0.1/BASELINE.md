# Baseline before the CSP4CMSIS 2.0.1 update

**Date:** 2026-10-04. **Commit:** `7ea395c` (main, up to date with origin). **Library:** `lib/csp4cmsis/`,
the pre-1.0 FreeRTOS-native snapshot (tree `17c36e6`, as in The_Process and Processes_and_Channels).
**Build:** STM32CubeIDE 2.1.0 headless, Debug (`-O0`, GNU++17); Release does not build (no
`lib/csp4cmsis/inc` include path and no GNU++17 in Release: 15 errors). **Board:** NUCLEO-G474RE,
ST-LINK-V3 VCP (LPUART1) 115200. Button interrupts injected over SWD (EXTI `SWIER1` bit 13): the real
EXTI/ISR path, with the pin read as "not pressed", so each is a release event.

## Interrupt path (as found)

| Item | Finding |
|---|---|
| Source | B1 (blue button) on PC13, EXTI line 13, `EXTI15_10_IRQn`; `EXTI15_10_IRQHandler` -> `BSP_PB_IRQHandler` -> `BSP_PB_Callback` (main.c `USER CODE 4`) -> `csp_send_button_event(pressed)` |
| Edges | both (read on the board: RTSR1 = FTSR1 = 1), but only because the ST BSP driver was **edited by hand** in the initial commit: `Drivers/BSP/STM32G4xx_Nucleo/stm32g4xx_nucleo.c`, `BSP_PB_Init()`, `GPIO_MODE_IT_RISING` -> `GPIO_MODE_IT_RISING_FALLING` (FW_G4 V1.6.1's original has rising only; a CubeMX regeneration restores it, and then only presses are reported: no release, no trigger, no counter). The `USER CODE SysInit` block that "changes it to both edges" (pull-up, priority 15/15) runs before CubeMX's own `BSP_PB_Init` call, which overrides it: dead code |
| NVIC priority | **15** at run time (read on the board), set by the BSP (`BSP_BUTTON_USER_IT_PRIORITY 15U` in `stm32g4xx_nucleo_conf.h`); numerically >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5): valid. The `.ioc` NVIC entry (priority 5, uses FreeRTOS functions) is not what the code applies |
| Channel | `Channel<ButtonEvent>` (rendezvous, Block), `ButtonEvent { bool pressed; }`, written with `writer().putFromISR()`; the return value is ignored |
| ISR semantics (old library) | the event is delivered only if ButtonProcess is already waiting in `in >> ev`; otherwise it is **dropped** and `putFromISR` returns false. `portYIELD_FROM_ISR` is inside the library, not in the application |
| ButtonProcess | prints "Blue button pressed" / "Blue button released"; on release it sends a trigger (rendezvous) to Sender, which sends a counter (rendezvous) to Receiver, which prints `Send: n Received: n` |
| Other | USART1 (PC4/PC5) is configured in the `.ioc` and initialised but unused; the console is LPUART1 (BSP COM1). `main.c` is double-spaced (every line followed by an empty line) |

## Board results (Debug)

| | |
|---|---|
| ELF SHA-256 | `e8c0260557c904e46d6753806cb8da27a34e2a61c3af8f9e11c2bad1f35ff7f4` |
| text / data / bss | 46 232 / 132 / 43 436 B |
| UART, no button | welcome line, bootstrap banner, `--- Single Sender & Receiver + Button ISR ---` |
| UART, 3 single interrupts (3 s apart) | 3 × (`Blue button released`, `Send: n Received: n`), n = 0, 1, 2 |
| Stacks (`CSProcessStatic<512>`, 2048 B each) | ButtonProcess 344 B, Sender 216 B, Receiver 520 B used |
| Priorities ButtonProcess / Sender / Receiver | 2 / 2 / 2 (MainApp 3, defaultTask 24) |
| FreeRTOS heap (heap_4, 30 720 B) | 7 allocations, 2 frees (MainApp freed by the idle task), 28 280 B free, 19 976 B minimum ever free |
| newlib `_sbrk` | 1032 B |

## Burst test (instrumented scratch build, not committed: `burst_patch.py`)

ButtonEvent gets a sequence number; the ISR counts failed `putFromISR` calls; ButtonProcess records
the sequence numbers it receives. MainApp, 2 s after start: one interrupt (ButtonProcess is waiting),
`vTaskDelay(1)` (ButtonProcess starts printing), then 10 interrupts back to back while it prints.

| Run | Interrupts | putFromISR failures | Events delivered | Delivered sequence numbers |
|---|---|---|---|---|
| 1, 2, 3 | 11 | **10** | 1 | 1 |

The 10 interrupts during the busy period are **lost**, silently (the application ignores the return
value): with a rendezvous channel an interrupt can only hand over to a process that is already
waiting. Logs: `results/baseline_*`, `results/burst_old_*`.
