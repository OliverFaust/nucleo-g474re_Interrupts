// application.cpp

#include <cstdio>

#include "application.h"
#include "csp/csp4cmsis.h"

using namespace csp;

// --- Button event type ---
struct ButtonEvent {
  bool pressed;  // true = pressed, false = released
};

struct trigger_t {};

using MessageType = unsigned int;

// --- Channel for button events ---
static Channel<ButtonEvent> buttonChan;

static Channel<trigger_t> g_trigger_chan;
static Channel<MessageType> counterChan;

// --- C-callable function for the ISR ---
extern "C" void csp_send_button_event(bool pressed) {
  // putFromISR never blocks;
  buttonChan.writer().putFromISR(ButtonEvent{pressed});
}

class ButtonProcess : public CSProcessStatic<512> {
  Chanin<ButtonEvent> in;
  Chanout<trigger_t> out;

 public:
  ButtonProcess(Chanin<ButtonEvent> r, Chanout<trigger_t> w) : in(r), out(w) {}
  const char* name() const override { return "ButtonProcess"; }

  void run() override {
    ButtonEvent ev;
    trigger_t t;
    while (true) {
      in >> ev;
      if (ev.pressed) {
        printf("Blue button pressed\r\n");
      } else {
        printf("Blue button released\r\n");
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
      printf("Send: %u Received: %u\r\n", received, received);
    }
  }
};

void MainApp_Task(void* params) {
  vTaskDelay(pdMS_TO_TICKS(10));
  printf("\r\n--- Single Sender & Receiver + Button ISR ---\r\n");

  static ButtonProcess buttonProc(buttonChan.reader(), g_trigger_chan.writer());
  static Sender sender(g_trigger_chan.reader(), counterChan.writer());
  static Receiver receiver(counterChan.reader());

  Run(InParallel(sender, receiver, buttonProc), ExecutionMode::StaticNetwork);

  // Run() returns immediately in StaticNetwork mode; the task must
  // delete itself rather than fall off the end of the function.
  vTaskDelete(NULL);
}

void csp_app_main_init(void) {
  BaseType_t status = xTaskCreate(MainApp_Task, "MainApp", 2048, NULL,
                                  tskIDLE_PRIORITY + 3, NULL);
  if (status != pdPASS) {
    printf("ERROR: MainApp_Task creation failed!\r\n");
  }
}
