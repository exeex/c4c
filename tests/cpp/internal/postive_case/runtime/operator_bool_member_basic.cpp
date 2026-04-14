// Runtime test: operator bool implicit conversion in boolean contexts.
struct Handle {
  int fd;

  operator bool() {
    if (fd >= 0) return true;
    return false;
  }
};

int main() {
  Handle h;
  h.fd = 42;
  if (!h) return 1;        // should be truthy
  h.fd = -1;
  if (h) return 2;         // should be falsy
  h.fd = 0;
  if (!h) return 3;        // fd=0 is >= 0, truthy

  // Test in while context
  Handle w;
  w.fd = 2;
  int count = 0;
  while (w) {
    count = count + 1;
    w.fd = w.fd - 1;
  }
  // fd goes 2->1->0->-1, loop runs when fd>=0, so 3 iterations
  if (count != 3) return 4;

  return 0;
}
