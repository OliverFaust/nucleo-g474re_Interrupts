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
