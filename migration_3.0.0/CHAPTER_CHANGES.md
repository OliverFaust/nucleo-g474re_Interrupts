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
5. **Unchanged:** output, burst-test behaviour, priorities, interrupt configuration, memory (0 FreeRTOS heap
   allocations); stacks except ButtonProcess's +8 B in Debug.
