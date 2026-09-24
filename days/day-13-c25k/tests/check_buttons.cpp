#include "../firmware/c25k/buttons.h"

#include <cstdio>
#include <cstdlib>
#include <initializer_list>

using c25k::ButtonGrammar;
using c25k::Input;

struct Sample {
  uint32_t at;
  bool a, b, clickedA = false, clickedB = false;
  Input expected = Input::None;
};

static int assertions = 0;
static void check(const char* name, std::initializer_list<Sample> samples) {
  ButtonGrammar grammar;
  for (const auto& sample : samples) {
    const auto actual = grammar.update(sample.at, sample.a, sample.b,
                                       sample.clickedA, sample.clickedB);
    ++assertions;
    if (actual != sample.expected) {
      std::fprintf(stderr, "%s at %u: expected %d, got %d\n", name,
                   sample.at, int(sample.expected), int(actual));
      std::exit(1);
    }
  }
}

int main() {
  check("single clicks require release and rearm", {
    {0, false, false}, {10, false, true}, {40, false, true},
    {50, false, false, false, true, Input::Next},
    {60, true, false}, {90, false, false, true, false, Input::Prev},
    {100, false, false},
  });
  check("unobserved clicks do not create input", {
    {10, false, false, true, true},
  });
  check("single long holds do nothing", {
    {10, true, false}, {1000, true, false}, {1100, false, false},
    {1200, false, true}, {2200, false, true}, {2300, false, false},
  });
  check("exact join and overlap thresholds are accepted", {
    {10, true, false}, {90, true, true}, {169, true, true},
    {170, false, false, true, true, Input::Enter},
  });
  check("blue-first chord waits for both releases", {
    {0, false, true}, {50, true, true},
    {130, false, true, true, false},
    {200, false, false, false, true, Input::Enter},
    {210, false, false}, {220, false, true},
    {250, false, false, false, true, Input::Next},
  });
  check("yellow-first chord waits for both releases", {
    {0, true, false}, {50, true, true},
    {130, true, false, false, true},
    {200, false, false, true, false, Input::Enter},
  });
  check("brief overlap falls back to first even if first releases early", {
    {0, false, true}, {20, true, true},
    {99, true, false, false, true},
    {140, false, false, true, false, Input::Next},
    {150, false, false},
  });
  check("brief overlap waits for the first release", {
    {0, true, false}, {20, true, true},
    {99, true, false, false, true},
    {120, false, false, true, false, Input::Prev},
  });
  check("brief simultaneous overlap ties to yellow", {
    {0, true, true}, {79, false, false, true, true, Input::Prev},
  });
  check("brief overlap does not turn first long hold into a click", {
    {0, true, false}, {10, true, true}, {30, true, false, false, true},
    {700, true, false}, {710, false, false},
  });
  check("brief overlap remembers only the original first release", {
    {0, true, false}, {20, true, true}, {40, false, true, false, false},
    {50, true, true}, {60, false, true, true, false},
    {70, false, false, false, true},
  });
  check("holding the remaining button cannot convert Enter to Escape", {
    {0, true, true}, {80, false, true, true, false}, {1000, false, true},
    {1100, false, false, false, false, Input::Enter},
  });
  check("hold threshold begins when second button joins", {
    {0, true, false}, {80, true, true}, {600, true, true},
    {679, true, true}, {680, true, true, false, false, Input::Escape},
    {1000, true, true}, {1010, false, true, true, false},
    {1020, false, false, false, true}, {1030, true, false},
    {1060, false, false, true, false, Input::Prev},
  });
  check("599 ms is Enter even when M5 hold suppresses click flags", {
    {0, true, true}, {599, false, false, false, false, Input::Enter},
  });
  check("release at 600 ms is Escape", {
    {0, true, true}, {600, false, false, false, false, Input::Escape},
    {601, false, false},
  });
  check("late join never becomes a hold chord", {
    {0, true, false}, {81, true, true}, {700, true, true},
    {800, false, true}, {900, false, false},
  });
  check("late join swallows second and permits first click with other up", {
    {0, true, false}, {81, true, true},
    {100, true, false, false, true},
    {150, false, false, true, false, Input::Prev},
  });
  check("late join suppresses first click when second still down", {
    {0, false, true}, {81, true, true},
    {100, true, false, false, true}, {150, false, false, true, false},
  });
  check("different buttons crossing in one sample are not a chord", {
    {0, true, false}, {50, false, true, true, false},
    {100, false, false, false, true},
  });
  check("join and hold work across millis wrap", {
    {0xFFFFFFD0u, false, true}, {0x20u, true, true},
    {0x277u, true, true}, {0x278u, true, true, false, false, Input::Escape},
    {0x280u, false, true, true, false}, {0x290u, false, false, false, true},
  });
  check("overlap threshold works across millis wrap", {
    {0xFFFFFFD0u, true, true},
    {0x20u, false, false, true, true, Input::Enter},
  });
  check("late joins stay late across millis wrap", {
    {0xFFFFFFD0u, true, false}, {0x21u, true, true},
    {0x400u, true, true}, {0x410u, false, false},
  });
  std::printf("Button grammar: %d assertions passed.\n", assertions);
}
