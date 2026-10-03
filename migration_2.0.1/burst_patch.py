# Test-only instrumentation for the burst test (never committed). Usage: burst_patch.py <application.cpp> old|new
import sys
f, ver = sys.argv[1], sys.argv[2]; s = open(f).read()
def rep(a, b):
    global s
    assert s.count(a) == 1, a; s = s.replace(a, b)
rep('''struct ButtonEvent {
  bool pressed;  // true = pressed, false = released
};''', '''struct ButtonEvent {
  bool pressed;  // true = pressed, false = released
  uint32_t seq;  // BURST TEST ONLY: interrupt sequence number
};
#include "stm32g4xx.h"
extern "C" { volatile uint32_t t_isr, t_fail, t_deliv, t_seq[32], t_burst_done; }
static constexpr uint32_t BURST_N = 10;''')
if ver == 'old':
    rep('buttonChan.writer().putFromISR(ButtonEvent{pressed});',
        'uint32_t s = ++t_isr; if (!buttonChan.writer().putFromISR(ButtonEvent{pressed, s})) t_fail++;')
    delay1, delayK = 'vTaskDelay(1);', 'vTaskDelay(pdMS_TO_TICKS(2000));'
    rep('''  // Run() returns immediately in StaticNetwork mode; the task must
  // delete itself rather than fall off the end of the function.
  vTaskDelete(NULL);''', '''  @@BURST@@;
  vTaskDelete(NULL);''')
else:
    rep('g_button_isr.putFromISR(ButtonEvent{pressed});',
        'uint32_t s = ++t_isr; if (!g_button_isr.putFromISR(ButtonEvent{pressed, s})) t_fail++;')
    delay1, delayK = 'osDelay(1);', 'osDelay(2000);'
    rep('''  osThreadExit();
}''', '''  @@BURST@@;
  osThreadExit();
}''')
rep('''      in >> ev;
''', '''      in >> ev;
      if (t_deliv < 32) t_seq[t_deliv] = ev.seq;
      t_deliv++;
''')
burst = (f'{delayK}  /* settle */ EXTI->SWIER1 = (1u << 13);  /* event 1: ButtonProcess is waiting */ '
         f'{delay1}  /* ButtonProcess now prints */ for (uint32_t i = 0; i < BURST_N; i++) EXTI->SWIER1 = (1u << 13); '
         f't_burst_done = 1')
s = s.replace('@@BURST@@', burst)
open(f, 'w').write(s)
