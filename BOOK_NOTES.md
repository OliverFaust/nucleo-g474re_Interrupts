# Book notes

The source files deliberately contain no comments: the explanations are in the book, which refers to the code by line. This file keeps every comment removed from this chapter's code on 2026-10-12 (from commit `41d510a`), so that nothing is lost.

For each file, in file order: where the comment was (line numbers before the removal), the comment itself, and the code it explains, numbered as in the source file now, i.e. as in the book listing.

## `Core/Src/application.cpp`

1. **Line 1**: file header, naming the file; explains no particular line.

   ```cpp
   // application.cpp
   ```

2. **Line 7**, at the end of the code line; explains listing line 5:

   ```cpp
   // StaticTask_t
   ```

   ```cpp
   5  #include "FreeRTOS.h"
   ```

3. **Lines 12–14**, above the code; explains listing lines 10–12:

   ```cpp
   // Written by the interrupt: true = pressed, false = released. An interrupt cannot wait for a
   // partner, so it writes into a buffered channel. KeepNewest: an event that arrives while the
   // previous one is still waiting replaces it (contact bounce, quick presses).
   ```

   ```cpp
   10  static BufferedChannel<bool, 1, BufferPolicy::KeepNewest> buttonChan;
   11  static Channel<bool> triggerChan;
   12  static Channel<unsigned int> counterChan;
   ```

4. **Line 19**, above the code; explains listing lines 14–15:

   ```cpp
   // Called from the EXTI interrupt (main.c).
   ```

   ```cpp
   14  extern "C" void csp_send_button_event(bool pressed) {
   15    buttonChan.isrWriter().putFromISR(pressed);
   ```

5. **Line 36**, at the end of the code line; explains listing line 30:

   ```cpp
   // a release triggers the Sender
   ```

   ```cpp
   30  out << true;
   ```

6. **Line 69**, above the code; explains listing line 63:

   ```cpp
   // The only process that prints: two threads printing at once would lose characters.
   ```

   ```cpp
   63        printf("Blue button released: Send: %u Received: %u\r\n", received, received);
   ```

7. **Lines 75–76**, above the code; explains listing lines 68–69:

   ```cpp
   // MainApp runs above the network, so the processes first run after MainApp has printed
   // its banner and exited.
   ```

   ```cpp
   68  static constexpr osPriority_t MAIN_APP_PRIORITY = osPriorityBelowNormal;
   69  static constexpr osPriority_t NETWORK_PRIORITY  = osPriorityLow;
   ```

8. **Line 80**, above the code; explains listing lines 71–72:

   ```cpp
   // Static stack (384 words = 1.5 KB) and control block: no heap.
   ```

   ```cpp
   71  alignas(8) static uint32_t mainAppStack[384];
   72  static StaticTask_t mainAppControlBlock;
   ```
