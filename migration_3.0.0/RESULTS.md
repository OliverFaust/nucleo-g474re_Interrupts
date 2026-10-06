# CSP4CMSIS 3.0.0 update: results

**Date:** 2026-10-06. **Board, tools:** as for 2.0.1 (`../migration_2.0.1/RESULTS.md`). **Library:**
CSP4CMSIS v3.0.0 (commit `647a1cb`), unmodified. Procedure: `CHECKLIST.md` section 9 of
nucleo-g474re_The_Process. Scripts: `../migration_2.0.1/run.sh` (20 s run with 3 SWD-injected button
interrupts, SWD readings), `burst_patch.py`, `burst_read.sh` (unchanged; the process object layout is the
same in 3.0). Reference: the 2.0.1 logs `../migration_2.0.1/results/final_*`, `burst_new_result.txt`.

## Commits (branch csp4cmsis-3.0.0, from main `676f30a`)

| Commit | Change |
|---|---|
| lib/csp4cmsis | unmodified v3.0.0 sources, `LICENSE`, `VERSION` |
| Debug and Release defines | only `CSP4CMSIS_MAX_SYSCALL_INTERRUPT_PRIORITY=5` and `CSP4CMSIS_DEVICE_HEADER="stm32g4xx.h"` |
| `BufferedChannel<ButtonEvent, 1, BufferPolicy::KeepNewest>` | was `SamplingBufferedChannel<…>` (removed in 3.0); `buttonChan.isrWriter()` unchanged |
| README | 3.0.0, the two defines, `BufferedChannel` |

## Board results

| | 2.0.1 Debug | 3.0.0 Debug | 2.0.1 Release | 3.0.0 Release |
|---|---|---|---|---|
| Build | 0 errors, 0 warnings | 0 errors, 0 warnings | 0 errors, 0 warnings | 0 errors, 0 warnings |
| text / data / bss (B) | 48 580 / 132 / 18 512 | 48 416 / 132 / 18 512 | 28 204 / 112 / 18 424 | 28 080 / 112 / 18 424 |
| UART, 20 s with 3 injected interrupts | banner, 3 × (`Blue button released`, `Send: n Received: n`) | **identical** | same | **identical** |
| Stacks used: ButtonProcess / Sender / Receiver / MainApp / defaultTask | as `final_debug_swd.txt` | identical except ButtonProcess 348 B (was 340 B) of 2 KB | as `final_release_swd.txt` | **identical** |
| Priorities, FreeRTOS heap (0 allocations), `_sbrk`, EXTI edges, NVIC priority 15 | | identical | | identical |

- **ButtonProcess, Debug:** 8 B more stack in 3.0.0 (348 of 2048 B); cause not examined (library code at
  `-O0`). Release identical.
- **Static allocation without the define:** 0 FreeRTOS heap allocations; the buffered channel's semaphores
  are static (FreeRTOS detected from `FreeRTOS.h`).

## Burst test (instrumented scratch build, not committed; three runs, identical)

| | Interrupts | Write failures | Events delivered | Delivered sequence numbers |
|---|---|---|---|---|
| 2.0.1 | 11 | 0 | 2 | 1, 11 |
| 3.0.0 | 11 | 0 | 2 | 1, 11 |

`results/burst_v300_result.txt`, `burst_v300_*_uart.txt`.

## Regeneration and fresh clone

- **GENERATE CODE** on the committed `.ioc`: no change in git; rebuilt ELFs byte-identical (Debug
  `0daee095…`, Release `0c22962d…`, as on the board).
- **Fresh clone** to another path, empty workspace, import, build (0 errors, 0 warnings both): Release ELF
  byte-identical, Debug flash image identical; flashed and run with 3 injected interrupts: output identical
  (`results/fresh_debug_irq_uart.txt`).
