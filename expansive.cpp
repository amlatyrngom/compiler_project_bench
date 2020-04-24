#include "expansive.h"

int64_t Fib(int64_t arg) {
  if (arg <= 1) return arg;
  return Fib(arg - 1) + Fib(arg - 2);
}
