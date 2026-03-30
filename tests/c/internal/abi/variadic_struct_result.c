#include <stdarg.h>

struct record {
  long id;
  double value;
};

static struct record pick_record(int index, ...) {
  va_list ap;
  va_start(ap, index);
  struct record chosen = {-1, -1.0};
  for (int i = 0; i <= index; ++i) {
    chosen = va_arg(ap, struct record);
  }
  va_end(ap);
  return chosen;
}

int main(void) {
  const struct record r0 = {10, 1.5};
  const struct record r1 = {20, 2.5};
  const struct record r2 = {30, 3.5};
  struct record picked = pick_record(2, r0, r1, r2);
  if (picked.id != r2.id) return 1;
  return picked.value == r2.value ? 0 : 1;
}
