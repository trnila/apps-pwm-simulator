#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <vector>
#include <stdint.h>
#include <sys/time.h>
#include <thread>
#include <cstring>

typedef enum {
  PTC0,
  PTC1,
  PTC2,
  PTC3,
  PTC4,
  PTC5,
  PTC7,
  PTC8,
	PinLast,
} PinName;

const char *pin_names[] {
	"PTC0",
	"PTC1",
	"PTC2",
	"PTC3",
	"PTC4",
	"PTC5",
	"PTC7",
	"PTC8",
};

struct Pin {
  uint64_t start_t;
  uint64_t change_t;
  int prev_state;
  int cur_state;
  uint32_t period_ms;
  uint32_t duty_ms;
};

struct Pin __pins[PinLast];
uint64_t __tick;
FILE *__csv;

class DigitalOut {
private:
	PinName name;

  public:
  DigitalOut(PinName name): name(name) {}

  DigitalOut& operator= (int value) {
		__pins[name].cur_state = value;
    return *this;
  }
};

typedef void (*Callback)();
typedef struct {
  Callback cb;
  uint64_t tick;
} RegisteredCb;

std::vector<RegisteredCb> callbacks;

class Ticker {
  public:
    void attach(void (*fn)(), float sec) {
      callbacks.push_back({fn, (uint64_t) (sec * 1000)});
    }

    void attach_us(void (*fn)(), uint64_t usec) {
      callbacks.push_back({fn, usec / 1000});
    }
};

void wait(float sec) {
  usleep(sec * 1000000);
}

void ticker_thread() {
  char line[128];
	for(;;) {
		__tick++;
		for(auto &item: callbacks) {
			if(__tick % item.tick == 0) {
				item.cb();
			}
		}

    int len = 0;
		for(int i = 0; i < PinLast; i++) {
      struct Pin *pin = &__pins[i];

      if(pin->prev_state == 0 && pin->cur_state == 1) {
        pin->period_ms = __tick - pin->start_t;
        pin->duty_ms = pin->period_ms - (__tick - pin->change_t);
        pin->start_t = __tick;

      } else if(pin->prev_state == 1 && pin->cur_state == 0) {
        pin->change_t = __tick;
      }

      pin->prev_state = pin->cur_state;

      len += sprintf(line + len, "%d", pin->cur_state);
			if(i + 1 != PinLast) {
        line[len++] = ',';
			}
		}
    line[len++] = '\n';
    fwrite(line, len, 1, __csv);
    fflush(__csv);

		for(int i = 0; i < PinLast; i++) {
      struct Pin *pin = &__pins[i];
      int active_percent = 0;
      if(pin->period_ms != 0) {
        active_percent = pin->duty_ms * 100 / pin->period_ms;
      }

      if(__tick - pin->start_t > 100) {
        printf("--/-- (%3d%%)   ", pin->cur_state ? 100 : 0);
      } else {
        printf("%2d/%2d (%3d%%)   ", pin->duty_ms, pin->period_ms, active_percent);
      }
    }
    printf("\n");

		usleep(1000);
	}

}

int mmain();
int main() {
	__csv = fopen("record.csv", "w");
	for(int i = 0; i < PinLast; i++) {
		fprintf(__csv, "%s", pin_names[i]);
		if(i + 1 != PinLast) {
			fprintf(__csv, ",");
		}
	}
	fprintf(__csv, "\n");

	std::thread t(ticker_thread);
  mmain();
  printf("main returned...\n");
}

#define main mmain
