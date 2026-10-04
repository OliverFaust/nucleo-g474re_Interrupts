# CSP4CMSIS 2.0.1 update: results

**Date:** 2026-10-04. **Board:** NUCLEO-G474RE (ST-LINK-V3, VCP = LPUART1, 115200). **Tools:** STM32CubeIDE
2.1.0 (GNU Tools for STM32 14.3.1, headless build), STM32CubeMX 6.17.0, FW_G4 V1.6.3 (FreeRTOS 10.3.1),
STM32CubeProgrammer (flash; SWD reads and writes in hotplug mode, no reset). **Library:** CSP4CMSIS
v2.0.1, unmodified. Procedure: `CHECKLIST.md` of nucleo-g474re_The_Process. Baseline: `BASELINE.md`.
**Button interrupts** were injected over SWD (EXTI `SWIER1` bit 13: the real EXTI and ISR path; the pin
reads "not pressed", so each is a release). The physical button was not pressed in these tests.
Scripts: `run.sh` (board run with 3 injected interrupts, SWD readings), `measure.py`, `burst_patch.py`
and `burst_read.sh` (burst test).

## Commits (branch csp4cmsis-2.0.1, from main `7ea395c`, which was up to date with origin)

| Commit | Change |
|---|---|
| Baseline | `BASELINE.md`, logs, scripts |
| Prepare for regeneration | heap (30 720, unchanged) and `USE_NEWLIB_REENTRANT` in the `.ioc`; bootstrap and `<stdbool.h>` into USER CODE; **BSP driver back to ST's original** (it had been hand-edited for the falling edge), falling edge added in `USER CODE BSP`; dead `USER CODE SysInit` block removed; interim button hook in `stm32g4xx_it.c` |
| Migrate to FW_G4 V1.6.3 | 34 HAL/CMSIS/BSP files; `Middlewares/` (FreeRTOS 10.3.1) unchanged; V1.6.3's `BSP_PB_Init()` still rising edge only |
| Button interrupt: register the application's EXTI callback | `HAL_EXTI_RegisterCallback(&hpb_exti[BUTTON_USER], …, ButtonExtiCallback)` in `USER CODE BSP`, callback in `USER CODE 4`; `stm32g4xx_it.c` as generated |
| main.c layout | default USER CODE sections back to CubeMX's layout (were double-spaced; whitespace only) |
| CubeMX: static defaultTask; configASSERT | as in The_Process |
| CSP4CMSIS 2.0.1 | library + LICENSE + VERSION; include path, defines (Debug and Release, + GNU++17 in Release); `application.cpp`: KeepNewest capacity-1 buffered channel written via `isrWriter()`, CMSIS-RTOS2 bootstrap, priorities; MainApp stack 1.5 KB and FreeRTOS heap 1 KB from the measurements |
| Untrack language.settings.xml; LICENSE; README | as in The_Process; LICENSE "Copyright (c) 2026 Oliver Faust" (first commit 2026-03-30) |

## Findings beyond the plan

1. **The project did not survive regeneration in three ways that break the button.** Regenerating
   from the old `.ioc`:
   - restores ST's `BSP_PB_Init()` (rising edge only), so no release events are seen;
   - replaces the application's `BSP_PB_Callback()` with CubeMX's BSP demo callback, so no events
     reach CSP at all;
   - removes `#include <stdbool.h>`, so the project no longer compiles.

   All three are now handled in USER CODE sections.
2. **The BSP demo code cannot simply be turned off** (`NUCLEO-G474RE.Bsp_Common_DEMO=false`):
   - CubeMX then drops the `USER CODE BSP` section, the only one between the BSP initialisation and
     `osKernelStart()`;
   - the user code left in the main loop would still reference the deleted `BspButtonState`.

   So the demo stays on, and the application registers its own EXTI callback instead of the BSP's.
3. **Button IRQ priority:** the runtime priority is 15, read on the board.
   - It is fixed by CubeMX's BSP template (`BSP_BUTTON_USER_IT_PRIORITY 15U` in the generated
     `stm32g4xx_nucleo_conf.h`).
   - The `.ioc` NVIC entry for `EXTI15_10_IRQn` (5, "uses FreeRTOS functions") generates no code.
     CubeMX resets it to 5 when it is edited to 15.
   - Both values are numerically >= 5, so they are valid; no change was made (step 3d).
4. **CubeMX migration prompt:** since about 00:33 on 2026-10-04, STM32CubeMX 6.17.0 stops with a "New
   STM32Cube firmware version available" dialog when it loads a project on FW_G4 V1.6.1. Projects on
   V1.6.3 are not affected. Every regeneration after the migration commit was therefore done on V1.6.3.
5. **Concurrent console output loses characters (application/BSP, not CSP4CMSIS).**
   - ButtonProcess and Receiver both print, at the same priority.
   - The BSP's `__io_putchar()` transmits one character at a time and ignores `HAL_BUSY`, so a
     character printed while the other thread's transmission is in progress is dropped.
   - Measured in the 2.0.1 burst test: 21 characters dropped, exactly the line
     `Send: 0 Received: 0\r\n` (`results/burst_new_putchar_busy.txt`).
   - With events seconds apart (normal button use) it does not occur. The old version has the same
     exposure (for example, bounce arriving while the Receiver prints).
   - Documented in the README (Troubleshooting); not fixed (design decision: one printing process, a
     mutex around the console, or a waiting `__io_putchar`).

## Board results

| | Baseline (old library, Debug) | 2.0.1 Debug | 2.0.1 Release |
|---|---|---|---|
| Build | Debug only (Release: 15 errors) | 0 errors, 0 warnings | 0 errors, 0 warnings |
| text / data / bss (B) | 46 232 / 132 / 43 436 | 48 580 / 132 / 18 512 | 28 204 / 112 / 18 424 |
| UART, 3 single interrupts 3 s apart | banner, then 3 × (`Blue button released`, `Send: n Received: n`), n = 0, 1, 2 | **identical** | **identical** |
| EXTI line 13 edges / NVIC priority | rising + falling / 15 | rising + falling / 15 | rising + falling / 15 |
| Stacks: ButtonProcess, Sender, Receiver (2 KB each) | 344, 216, 520 B | 340, 320, 524 B | 308, 212, 492 B |
| MainApp stack | 8 KB from the heap (not measured) | 596 B of 1.5 KB (static) | 308 B of 1.5 KB |
| defaultTask stack (2 KB) | heap (not measured) | 128 B (static) | 100 B |
| Priorities processes / MainApp / defaultTask | 2 / 3 / 24 | 8 / 16 / 24 | 8 / 16 / 24 |
| FreeRTOS heap | 7 allocations, 2 frees | **0 allocations** (heap 1 KB) | **0 allocations** |
| newlib `_sbrk` | 1032 B | 1032 B | 1032 B |

## Burst test: 1 interrupt, then 10 back to back while ButtonProcess is printing

(Instrumented scratch builds, not committed: `burst_patch.py`. Three runs each, identical results.)

| | Interrupts | Write failures | Events delivered | Delivered sequence numbers | Console |
|---|---|---|---|---|---|
| Baseline: `Channel` (rendezvous) + `putFromISR` | 11 | **10** (ignored by the application) | 1 | 1 | `released`, `Send: 0` |
| 2.0.1: `SamplingBufferedChannel<…, 1, KeepNewest>` + `isrWriter()` | 11 | **0** | 2 | 1, **11** (the latest) | `released`, `released`, `Send: 1` (`Send: 0` lost by the console driver, finding 5) |

As expected:
- the old version lost every interrupt that arrived while ButtonProcess was busy, without notice;
- 2.0.1 merges them into one event carrying the latest value, and the ISR write never fails.

## Regeneration and fresh clone

- **GENERATE CODE** (CubeMX 6.17.0) on the committed `.ioc`: no change in git; rebuilt Debug and
  Release ELFs byte-identical to those flashed above, so the board output is identical.
- **Fresh clone** to another path, empty workspace, import, build (Debug and Release, 0 errors, 0
  warnings), flash:
  - Release ELF byte-identical;
  - Debug flash image identical (the ELF's debug information contains the build path);
  - board output and measurements identical (`results/fresh_debug_*`);
  - `language.settings.xml` was recreated by the import (ignored by git).

## What changes in the book chapter text (Interrupts)

**The ISR and the button channel** (the subject of the chapter):

1. **Channel type:**
   - `static Channel<ButtonEvent> buttonChan;` becomes
     `static SamplingBufferedChannel<ButtonEvent, 1, BufferPolicy::KeepNewest> buttonChan;` plus
     `static IsrChanout<ButtonEvent> buttonIsr = buttonChan.isrWriter();`;
   - the ISR function calls `buttonIsr.putFromISR(ButtonEvent{pressed})`, not
     `buttonChan.writer().putFromISR(...)`.
2. **Why an interrupt cannot use a rendezvous channel:**
   - a rendezvous needs both partners at the same time, and an interrupt cannot wait;
   - 2.0.1 therefore has no ISR write path on rendezvous channels.

   The old text ("`putFromISR` on a rendezvous channel is safe and non-blocking") must go. It was
   non-blocking only because it **dropped** the event, without telling anyone, whenever ButtonProcess
   was not already waiting. Measured: 10 of 11 burst interrupts lost.
3. **KeepNewest, capacity 1:**
   - the write never blocks and never fails;
   - events arriving while ButtonProcess is busy are **merged**: the latest press or release counts,
     and bounce is absorbed;
   - they are replaced, not queued and not lost unnoticed. Measured: of 11 burst interrupts,
     ButtonProcess received the first and the last.
4. **Element size:**
   - the event is copied with interrupts masked (BASEPRI);
   - `sizeof(T)` must not exceed `CSP4CMSIS_ISR_MAX_ELEMENT_SIZE` (64 B, compile-time check);
   - for large data, send an index into a static pool.
5. **No `portYIELD_FROM_ISR`:** the context switch is requested inside the library (CMSIS-RTOS2 wakeup
   from ISR). The old library did it inside too; no application code ever had it.
6. **Interrupt priority:**
   - an interrupt that calls CSP4CMSIS must have a priority numerically >= `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`
     (5), which equals `CSP4CMSIS_MAX_SYSCALL_INTERRUPT_PRIORITY`;
   - the button runs at 15, set by the BSP (`BSP_BUTTON_USER_IT_PRIORITY`), not by the `.ioc` NVIC
     entry.
7. **The interrupt path in `main.c`:**
   - CubeMX's `BSP_PB_Init()` enables the rising edge only; the falling edge is added in
     `USER CODE BSP`;
   - `ButtonExtiCallback()` (in `USER CODE 4`) is registered with `HAL_EXTI_RegisterCallback()`,
     instead of overriding `BSP_PB_Callback()`, which CubeMX's BSP demo code generates;
   - the old SysInit reconfiguration (pull-up, priority 15/15, "rising edge only after
     BSP_PB_Init") is gone; it never took effect;
   - the trigger is sent on **release** (the old README said press in one place).

**Processes, start-up, project** (as in The_Process and Processes_and_Channels):

8. **`application.cpp`:**
   - `osThreadNew` with a static 1.5 KB stack and control block, with the measured use in the
     comment;
   - `osDelay(10)`, `osThreadExit()`;
   - includes `cmsis_os2.h` and `FreeRTOS.h` (for `StaticTask_t`);
   - priorities `osPriorityLow` (processes, 8) < `osPriorityBelowNormal` (MainApp, 16) <
     `osPriorityNormal` (defaultTask, 24), formerly 2 < 3 < 24, with the start-order comment;
   - `Run(InParallel(...), ExecutionMode::StaticNetwork, NETWORK_PRIORITY)`.
9. **Stack units:** `CSProcessStatic<512>` counts words (2 KB), while `osThreadAttr_t.stack_size` counts
   bytes (MainApp 384 words = 1.5 KB, measured 596 B). The old `xTaskCreate(..., 2048, ...)` meant
   8 KB.
10. **Rendezvous channels `g_trigger_chan` and `counterChan`:**
    - unchanged;
    - the 2.0.1 rules: the element type must be trivially copyable (`trigger_t` and `unsigned int`
      are), Block policy only, no ISR write.
11. **CubeMX settings:**
    - `USE_NEWLIB_REENTRANT` Enabled;
    - heap 1024 B in the `.ioc`;
    - `defaultTask` Allocation Static;
    - the printing `configASSERT`;
    - BSP demo code stays on (and why);
    - FW_G4 V1.6.3, CubeMX 6.17.0, CubeIDE 2.1.0.
12. **Project setup:**
    - `lib/csp4cmsis/` holds CSP4CMSIS 2.0.1, unmodified (`VERSION`, `LICENSE`);
    - include path `../lib/csp4cmsis/inc`;
    - the four defines and GNU++17 in Debug and Release;
    - the ST BSP drivers are not edited.
13. **Memory:**
    - no FreeRTOS heap allocation (measured; heap 1 KB); the buffered channel's semaphores are static
      too;
    - newlib's `printf` takes a 1 KB `stdout` buffer;
    - stack figures as measured.
14. **Console:**
    - LPUART1 via the ST-LINK virtual COM port, not USART1 (USART1 is configured in the `.ioc` but
      unused);
    - two processes print: output can lose characters if they print at the same moment (finding 5).

Logs: `results/`.
