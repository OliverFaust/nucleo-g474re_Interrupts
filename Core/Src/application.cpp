// application.cpp

#include <cstdio>

#include "application.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"  // StaticTask_t
#include "csp/csp4cmsis.h"

using namespace csp;

// Written by the interrupt: true = pressed, false = released. An interrupt cannot wait for a
// partner, so it writes into a buffered channel. KeepNewest: an event that arrives while the
// previous one is still waiting replaces it (contact bounce, quick presses).
static BufferedChannel<bool, 1, BufferPolicy::KeepNewest> buttonChan;
static Channel<bool> triggerChan;
static Channel<unsigned int> counterChan;

// Called from the EXTI interrupt (main.c).
extern "C" void csp_send_button_event(bool pressed) {
  buttonChan.isrWriter().putFromISR(pressed);
}

class ButtonProcess : public CSProcessStatic<512> {
  Chanin<bool> in;
  Chanout<bool> out;

 public:
  ButtonProcess(Chanin<bool> r, Chanout<bool> w) : in(r), out(w) {}

  void run() override {
    bool pressed;
    while (true) {
      in >> pressed;
      if (!pressed) {
        out << true;  // a release triggers the Sender
      }
    }
  }
};

class Sender : public CSProcessStatic<512> {
  Chanin<bool> in;
  Chanout<unsigned int> out;

 public:
  Sender(Chanin<bool> r, Chanout<unsigned int> w) : in(r), out(w) {}

  void run() override {
    unsigned int counter = 0;
    bool trigger;
    while (true) {
      in >> trigger;
      out << counter++;
    }
  }
};

class Receiver : public CSProcessStatic<512> {
  Chanin<unsigned int> in;

 public:
  explicit Receiver(Chanin<unsigned int> r) : in(r) {}

  void run() override {
    unsigned int received;
    while (true) {
      in >> received;
      // The only process that prints: two threads printing at once would lose characters.
      printf("Blue button released: Send: %u Received: %u\r\n", received, received);
    }
  }
};

// MainApp runs above the network, so the processes first run after MainApp has printed
// its banner and exited.
static constexpr osPriority_t MAIN_APP_PRIORITY = osPriorityBelowNormal;
static constexpr osPriority_t NETWORK_PRIORITY  = osPriorityLow;

// Static stack (384 words = 1.5 KB) and control block: no heap.
alignas(8) static uint32_t mainAppStack[384];
static StaticTask_t mainAppControlBlock;

void MainApp_Task(void* argument) {
  (void)argument;
  osDelay(10);
  printf("\r\n--- Single Sender & Receiver + Button ISR (Zero-Heap) ---\r\n");

  static ButtonProcess buttonProc(buttonChan.reader(), triggerChan.writer());
  static Sender sender(triggerChan.reader(), counterChan.writer());
  static Receiver receiver(counterChan.reader());

  Run(InParallel(sender, receiver, buttonProc), ExecutionMode::StaticNetwork, NETWORK_PRIORITY);
  osThreadExit();
}

void csp_app_main_init(void) {
  osThreadAttr_t attr = {};
  attr.name       = "MainApp";
  attr.stack_mem  = mainAppStack;
  attr.stack_size = sizeof(mainAppStack);
  attr.cb_mem     = &mainAppControlBlock;
  attr.cb_size    = sizeof(mainAppControlBlock);
  attr.priority   = MAIN_APP_PRIORITY;
  if (osThreadNew(MainApp_Task, NULL, &attr) == NULL) {
    printf("ERROR: MainApp_Task creation failed!\r\n");
  }
}
