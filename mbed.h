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

uint32_t on[PinLast];
uint32_t off[PinLast];
int states[PinLast];
uint64_t tick;
FILE *csv;

class DigitalOut {
private:
	PinName name;

  public:
  DigitalOut(PinName name): name(name) {}

  DigitalOut& operator= (int value) {
		states[name] = value;
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
      callbacks.push_back({fn, sec * 1000});
    }
};

void wait(float sec) {
  usleep(sec * 1000000);
}



void ticker_thread() {
  char line[128];
	for(;;) {
		tick++;
		for(auto &item: callbacks) {
			if(tick % item.tick == 0) {
				item.cb();
			}
		}

    int len = 0;
		for(int i = 0; i < PinLast; i++) {
			if(states[i]) {
				on[i]++;
			} else {
				off[i]++;
			}
      len += sprintf(line + len, "%d", states[i]);
			if(i + 1 != PinLast) {
        line[len++] = ',';
			}
		}
    line[len++] = '\n';
    fwrite(line, len, 1, csv);
    fflush(csv);

		if(tick % 100 == 0) {
			for(int i = 0; i < PinLast; i++) {
				int value = ((float) on[i]) / (off[i] + on[i]) * 100;
				printf("%3d ", value);
			}
			printf("\n");
			memset(on, 0, sizeof(on));
			memset(off, 0, sizeof(off));
		}

		usleep(1000);
	}

}

int mmain();
int main() {
	csv = fopen("record.csv", "w");
	for(int i = 0; i < PinLast; i++) {
		fprintf(csv, "%s", pin_names[i]);
		if(i + 1 != PinLast) {
			fprintf(csv, ",");
		}
	}
	fprintf(csv, "\n");

	std::thread t(ticker_thread);
  mmain();
  printf("main returned...\n");
}

#define main mmain
