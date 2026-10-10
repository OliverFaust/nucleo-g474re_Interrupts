# What changes in the book chapter text (Interrupts), 2.0.1 -> 3.0.0

1. **`application.cpp` listing:** `static BufferedChannel<ButtonEvent, 1, BufferPolicy::KeepNewest>
   buttonChan;` replaces `static SamplingBufferedChannel<ButtonEvent, 1, BufferPolicy::KeepNewest>
   buttonChan;`. In 3.0 `BufferedChannel<T, N, P>` is the one buffered channel (`P` defaults to `Block`);
   `SamplingBufferedChannel` is gone. The ISR writer (`buttonChan.isrWriter()`, `IsrChanout<T>`,
   `putFromISR()`) and the rest of the listing are unchanged.
2. **Text that explains the channel:** "a `SamplingBufferedChannel` with the KeepNewest policy" becomes "a
   `BufferedChannel` with the KeepNewest policy"; the explanation (an interrupt cannot wait for a partner,
   so it writes into a buffered channel; KeepNewest: merged, replaced, not queued) is unchanged.
3. **Project setup:** two defines in both configurations (`CSP4CMSIS_MAX_SYSCALL_INTERRUPT_PRIORITY=5`,
   `CSP4CMSIS_DEVICE_HEADER="stm32g4xx.h"`); static allocation is the default (the channel's semaphores are
   static without a define) and FreeRTOS is detected. A shifted value such as 0x50 for the priority define
   does not compile in 3.0.
4. **Library version:** CSP4CMSIS 3.0.0 (stable 3.x API), unmodified in `lib/csp4cmsis/`.
5. **Unchanged:** burst-test behaviour, priorities, interrupt configuration, memory (0 FreeRTOS heap
   allocations); stacks except ButtonProcess's +8 B in Debug.
6. **The console has one owner (branch `console-owner`):** ButtonProcess no longer prints (`Blue button
   pressed` and `Blue button released` are gone from its listing); the Receiver, the end of the pipeline,
   prints `Blue button released: Send: n Received: n`, one line per value (each value stands for one
   release: one trigger per release, one value per trigger). A press is no longer shown: it drives nothing
   in the network, and showing it would need a second input to the Receiver (an ALT, introduced in the next
   chapter) or a wider message type through the Sender. Text: the console is a shared resource; the BSP's
   console driver silently drops the characters of a second thread that prints while the UART is busy (the
   2.0.1 and 3.0.0 burst tests lost `Send: 0 Received: 0` this way), so one process owns it. The output
   listing and the troubleshooting entries change accordingly; in the burst test both lines now appear.
7. **No heap at all (branch `unbuffered-stdout`):** `main.c` (USER CODE 2) makes `stdout` unbuffered with
   `setvbuf(stdout, NULL, _IONBF, 0)`, so newlib's `printf()` no longer allocates its 1 KB `stdout` buffer:
   `_sbrk()` is never called (measured), as in Alternation and the Sensor chapter. The start-up banner
   becomes `--- Single Sender & Receiver + Button ISR (Zero-Heap) ---` (as in those two chapters), and the text can say that the program
   allocates no heap memory at all (FreeRTOS heap: 0 allocations; C library heap: not used).
8. **Simplified listing (branch `simplify-chapter-code`):** the code shows the chapter's concept and
   nothing else.
   - `struct ButtonEvent { bool pressed; }` is gone: `BufferedChannel<bool, 1, BufferPolicy::KeepNewest>
     buttonChan;` (`true` = pressed, `false` = released).
   - No `IsrChanout` variable: the interrupt callback calls `buttonChan.isrWriter().putFromISR(pressed);`
     (`isrWriter()` only wraps a pointer to the channel: no RTOS call, no state).
   - `struct trigger_t {}` is gone: the trigger channel is `Channel<bool> triggerChan;` (was
     `g_trigger_chan`), and ButtonProcess sends `out << true;` on a release.
   - `using MessageType = unsigned int;` is gone: `Channel<unsigned int> counterChan;`.
   - The three processes lose their `name()` overrides.
   - **`main.c`:** CubeMX's `defaultTask` is removed (its attributes, its creation, `StartDefaultTask`).
     The program's threads are now exactly the ones in `application.cpp` (MainApp and the processes),
     plus FreeRTOS's idle and timer tasks. The `.ioc` still contains the task: CubeMX does not allow a
     project without one and re-creates it on regeneration (from the `.ioc`, as the static, heap-free
     task it was); the README says to delete it again.
   - **Thread names:** without the `name()` overrides, the processes appear as `csp_task` in a
     debugger's thread view; MainApp keeps its name (`attr.name`).
   - **Comments** shortened to what is surprising; the explanations are in the chapter text.
   - The console output is unchanged.
