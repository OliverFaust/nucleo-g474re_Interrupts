# CSP4CMSIS Interrupt Demo for NUCLEO-G474RE

A real‑time embedded demonstration of the **CSP (Communicating Sequential Processes)** library CSP4CMSIS using CMSIS‑RTOS v2 on an STM32G474RE microcontroller. This project shows how to safely connect an external interrupt (blue user button) to CSP channels, triggering a chain of process communications. The formal CSP model is in [`Formal model/`](Formal%20model/).

## Features

- **FreeRTOS** with the CMSIS‑RTOS v2 API (STM32CubeMX `CMSIS_V2` interface)
- **CSP4CMSIS 3.0.0** library for channel‑based, deterministic concurrency
- **External interrupt** (blue button on PC13) on both edges: press and release
- **Interrupt to process communication** through the ISR writer end (`isrWriter()`) of a buffered channel
- **Trigger chain**: button release → ButtonProcess → Sender → Receiver
- **Roll‑over counter** sent from Sender to Receiver (unsigned integer)
- Zero heap: no FreeRTOS heap and no C library heap allocation (see [Memory](#memory))
- **Serial console output** via LPUART1, the ST‑LINK virtual COM port (115200 baud)

## Hardware Requirements

- STM32 Nucleo‑G474RE board  
- USB cable for power, programming, and serial communication  
- The blue user button (PC13) – no external components needed

## Software Requirements

Tested with:

| Tool | Version |
|---|---|
| STM32CubeIDE | 2.1.0 (GNU Tools for STM32 14.3.rel1) |
| STM32CubeMX (only to regenerate code) | 6.17.0 |
| STM32Cube FW_G4 | V1.6.3 (FreeRTOS 10.3.1) |
| CSP4CMSIS | 3.0.0, in `lib/csp4cmsis/` (unmodified; see `lib/csp4cmsis/VERSION`) |

## Serial Configuration

| Parameter   | Value          |
|-------------|----------------|
| Baud Rate   | 115200         |
| Data Bits   | 8              |
| Stop Bits   | 1              |
| Parity      | None           |
| Flow Control| None           |

## Building with STM32CubeIDE

1. **Clone this repository** (do not place it inside your STM32CubeIDE workspace directory).  
2. Open STM32CubeIDE.  
3. Go to `File → Import → Existing Projects into Workspace`.  
4. Select the cloned directory.  
5. Build the project (configuration `Debug` or `Release`).  
6. Flash the binary to your Nucleo board.

The CSP4CMSIS settings are already in the project (G++ compiler, Debug and Release): include path `../lib/csp4cmsis/inc`, and the two defines `CSP4CMSIS_MAX_SYSCALL_INTERRUPT_PRIORITY=5` and `CSP4CMSIS_DEVICE_HEADER="stm32g4xx.h"`; CSP4CMSIS allocates its RTOS objects statically by default and finds FreeRTOS from `FreeRTOS.h` (explained in the [CSP4CMSIS STM32CubeIDE guide](https://github.com/OliverFaust/CSP4CMSIS/blob/main/Documentation/CSP4CMSIS_STM32CubeIDE.md)).

## Regenerating code with STM32CubeMX

`nucleo-g474re_v10.ioc` can be opened and regenerated (GENERATE CODE) without losing anything: the application's code in `main.c` and `FreeRTOSConfig.h` sits between `USER CODE BEGIN`/`END` markers, and the FreeRTOS settings it needs (heap size, newlib reentrancy, static default task) are stored in the `.ioc`. The ST BSP drivers are used as CubeMX generates them.

## Project Structure
```text
├── Core/            # main.c (CubeMX + button setup), application.cpp (the example)
├── Drivers/         # STM32 HAL, CMSIS and BSP drivers
├── Formal model/    # CSP-M model
├── lib/csp4cmsis/   # CSP4CMSIS 3.0.0 (inc/, src/, LICENSE, VERSION)
├── Middlewares/     # FreeRTOS + CMSIS‑RTOS v2
├── nucleo-g474re_v10.ioc  # STM32CubeMX project
└── README.md
```

## How It Works

1. **Hardware interrupt**: the blue button is on PC13, EXTI line 13 (`EXTI15_10_IRQn`). CubeMX's `BSP_PB_Init()` enables the rising edge (press); `main.c` (`USER CODE BSP`) adds the falling edge (release) and registers `ButtonExtiCallback()` as the button's EXTI callback. The interrupt path is `EXTI15_10_IRQHandler` → `BSP_PB_IRQHandler` → `HAL_EXTI_IRQHandler` → `ButtonExtiCallback()`, which reads the pin and calls `csp_send_button_event(pressed)`. The interrupt priority is 15 (set by the BSP), numerically above `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` (5), as required for an interrupt that calls into CSP4CMSIS.

2. **Button channel**:
   `BufferedChannel<ButtonEvent, 1, BufferPolicy::KeepNewest> buttonChan;` – a buffered channel with room for one event, written by the interrupt through its ISR writer end, `buttonChan.isrWriter()`.
   - **Why not a rendezvous channel?** A rendezvous needs both partners to be ready at the same time, and an interrupt cannot wait. CSP4CMSIS therefore gives rendezvous channels no interrupt write path; an interrupt writes into a buffer.
   - **KeepNewest:** the write never blocks and never fails. If ButtonProcess is still busy with the previous event, a new event replaces the one waiting in the buffer: presses arriving in quick succession are **merged** into one event, and the latest press or release counts. This also absorbs contact bounce. Events are replaced, not queued: when ButtonProcess is ready again it gets the most recent state of the button, not the history.
   - The element type must be small (`sizeof(ButtonEvent)` ≤ 64 bytes, checked at compile time): it is copied with interrupts masked.

3. **ButtonProcess**:
   Waits on `buttonChan`. On a **release** it sends a `trigger_t` message to the Sender through `g_trigger_chan` (a rendezvous channel); a press sends nothing. It does not print.

4. **Sender**:
   Waits for a trigger on `g_trigger_chan`. Each trigger causes the Sender to send an ever‑incrementing `unsigned int` (which rolls over from `UINT_MAX` to `0`) to the Receiver through `counterChan` (a rendezvous channel).

5. **Receiver**:
   Waits on `counterChan`, reads the number, and prints `Blue button released: Send: X Received: X` (the received value, twice). Each value stands for one release: ButtonProcess sends one trigger per release, and the Sender one value per trigger.

6. **Start-up**: `main.c` calls `csp_app_main_init()`, which creates the `MainApp` thread (static 1.5 KB stack). `MainApp` prints the banner, starts the three processes with `Run(InParallel(sender, receiver, buttonProc), ExecutionMode::StaticNetwork, osPriorityLow)` and exits. `MainApp` runs at a higher priority (`osPriorityBelowNormal`), so the processes first run after it has exited.

## Example Console Output

```text
Welcome to STM32 world !

=== STM32 FreeRTOS + CSP4CMSIS bootstrap ===

--- Single Sender & Receiver + Button ISR (Zero-Heap) ---
Blue button released: Send: 0 Received: 0
Blue button released: Send: 1 Received: 1
Blue button released: Send: 2 Received: 2
...
```
Each release of the blue button triggers a counter increment, and the Receiver prints one line for it. The counter continues indefinitely, rolling over automatically.

**The console has one owner:** it is a shared resource, and the BSP's console driver (`__io_putchar()`) silently drops the characters of a second thread that prints while the UART is busy, so only the Receiver, the end of the pipeline, prints while the network runs (MainApp prints its banner before the processes start, at a higher priority).

## Memory

Measured on the board (Debug and Release):

- **FreeRTOS heap: not used.** `pvPortMalloc()` is never called (0 allocations). `ButtonProcess`, `Sender`, `Receiver`, `MainApp`, CubeMX's `defaultTask`, and FreeRTOS's idle and timer tasks all have static stacks and control blocks; the buffered button channel's semaphores are static too (CSP4CMSIS's default, static allocation), and the rendezvous channels need no RTOS objects. The FreeRTOS heap (`configTOTAL_HEAP_SIZE`) is therefore set to only 1 KB: enough for one small dynamically created thread (a 128‑word stack and its control block) if you switch one back to dynamic allocation.
- **C library heap: not used.** `main.c` (USER CODE 2) makes `stdout` unbuffered with `setvbuf(stdout, NULL, _IONBF, 0)`; otherwise newlib's `printf()` would `malloc()` a 1 KB `stdout` buffer on first use (measured: 1032 B). With it, `_sbrk()` is never called.
- So the program allocates no heap memory at all: the "(Zero-Heap)" in the start-up banner is literal.
- **Stacks used** (Debug; Release in brackets): `ButtonProcess` 340 B (308 B), `Sender` 320 B (212 B), `Receiver` 524 B (492 B), each of 2 KB; `MainApp` 596 B (308 B) of 1.5 KB; `defaultTask` 128 B (100 B) of 2 KB.

## Key CSP4CMSIS Concepts Demonstrated

- **Buffered channel with a policy** – `BufferedChannel<T, 1, KeepNewest>`: the latest value counts.
- **ISR writer end** – `isrWriter()`: the only way for an interrupt to send into the network; never blocks.
- **Channel** – synchronous rendezvous between processes (`g_trigger_chan`, `counterChan`).
- **Static network** – processes created once at start-up, with static stacks and control blocks, running for ever.
- **Process composition** – `InParallel` combines independent processes.
- **External choice** (not used here, but supported via `Alternative`).

## Troubleshooting

- **No output on serial**: Verify the baud rate and that the correct COM port (the ST‑LINK virtual COM port) is used.
- **No output when the button is released**: the falling edge is not enabled. It is set in `main.c`, `USER CODE BSP`, after CubeMX's `BSP_PB_Init()` (which enables the rising edge only).
- **Fewer lines than button releases when the button is pressed very rapidly**: by design. Events that arrive while ButtonProcess is still busy are merged (KeepNewest), so several quick releases can produce one trigger.
- **`configASSERT failed: <file>:<line>`** on the console: a FreeRTOS assertion failed at that source line; the program halts there.

## License and Declaration

MIT License – see the `LICENSE` file. CSP4CMSIS: MIT License, `lib/csp4cmsis/LICENSE`.

Development of this project utilizes AI coding assistants for boilerplate generation, unit test creation, and architectural drafting. All core logic is manually reviewed and verified.

## Acknowledgments

- STMicroelectronics for the STM32 HAL and CMSIS‑RTOS v2
- The FreeRTOS team
- [CSP4CMSIS](https://oliverfaust.github.io/CSP4CMSIS/) library by Oliver Faust
