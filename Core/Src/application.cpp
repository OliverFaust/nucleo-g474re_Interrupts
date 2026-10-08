// application.cpp

#include <cstdio>

#include "application.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"  // StaticTask_t: the control block of a statically created thread
#include "csp/csp4cmsis.h"

using namespace csp;

// --- Button event type ---
struct ButtonEvent {
  bool pressed;  // true = pressed, false = released
};

struct trigger_t {};

using MessageType = unsigned int;

// --- Channel for button events: written by the interrupt ---
// An interrupt cannot use a rendezvous channel: it cannot wait for a partner. It writes into
// a buffered channel instead, through the channel's ISR writer end. Capacity 1 with the
// KeepNewest policy: the write never blocks and never fails; events that arrive while
// ButtonProcess is still busy with the previous one replace each other, so the latest press
// or release counts (this also absorbs contact bounce).
static BufferedChannel<ButtonEvent, 1, BufferPolicy::KeepNewest> buttonChan;
static IsrChanout<ButtonEvent> buttonIsr = buttonChan.isrWriter();

static Channel<trigger_t> g_trigger_chan;
static Channel<MessageType> counterChan;

// --- C-callable function for the ISR ---
extern "C" void csp_send_button_event(bool pressed) {
  buttonIsr.putFromISR(ButtonEvent{pressed});  // never blocks; with KeepNewest always succeeds
}

class ButtonProcess : public CSProcessStatic<512> {
  Chanin<ButtonEvent> in;
  Chanout<trigger_t> out;

 public:
  ButtonProcess(Chanin<ButtonEvent> r, Chanout<trigger_t> w) : in(r), out(w) {}
  const char* name() const override { return "ButtonProcess"; }

  // ButtonProcess does not print: the console is a shared resource, and only the Receiver
  // uses it while the network runs. A press sends nothing; a release sends the trigger.
  void run() override {
    ButtonEvent ev;
    trigger_t t;
    while (true) {
      in >> ev;
      if (!ev.pressed) {
        out << t;
      }
    }
  }
};

class Sender : public CSProcessStatic<512> {
  Chanin<trigger_t> in;
  Chanout<MessageType> out;

 public:
  Sender(Chanin<trigger_t> r, Chanout<MessageType> w) : in(r), out(w) {}
  const char* name() const override { return "Sender"; }

  void run() override {
    unsigned int counter = 0;
    trigger_t t;
    while (true) {
      in >> t;
      out << counter;
      counter++;
    }
  }
};

class Receiver : public CSProcessStatic<512> {
  Chanin<MessageType> in;

 public:
  Receiver(Chanin<MessageType> r) : in(r) {}
  const char* name() const override { return "Receiver"; }

  void run() override {
    MessageType received;
    while (true) {
      in >> received;
      // Each value stands for one button release: ButtonProcess sends one trigger per
      // release, and the Sender one value per trigger.
      printf("Blue button released: Send: %u Received: %u\r\n", received, received);
    }
  }
};

// Start order. MainApp runs at a higher priority than the network it launches, so
// Run(..., StaticNetwork) only creates the three process threads and returns: none of them
// can preempt MainApp, and they first run after MainApp has printed its banner and exited.
// All stay below CubeMX's defaultTask (osPriorityNormal), as before.
static constexpr osPriority_t MAIN_APP_PRIORITY = osPriorityBelowNormal;
static constexpr osPriority_t NETWORK_PRIORITY  = osPriorityLow;

// MainApp's stack and control block are static: creating the thread takes no heap.
// CMSIS-RTOS2 counts the stack in bytes: 384 words = 1.5 KB. Measured on the NUCLEO-G474RE:
// MainApp uses 596 B (Debug, -O0) and 308 B (Release, -Os) of it.
alignas(8) static uint32_t mainAppStack[384];
static StaticTask_t mainAppControlBlock;

void MainApp_Task(void* argument) {
  (void)argument;
  osDelay(10);
  printf("\r\n--- Single Sender & Receiver + Button ISR (Zero-Heap) ---\r\n");

  static ButtonProcess buttonProc(buttonChan.reader(), g_trigger_chan.writer());
  static Sender sender(g_trigger_chan.reader(), counterChan.writer());
  static Receiver receiver(counterChan.reader());

  Run(InParallel(sender, receiver, buttonProc), ExecutionMode::StaticNetwork, NETWORK_PRIORITY);

  // Run() returns immediately in StaticNetwork mode; the thread must
  // end itself rather than fall off the end of the function.
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
