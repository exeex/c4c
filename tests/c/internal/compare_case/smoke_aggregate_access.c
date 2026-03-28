// Compare-mode smoke test: aggregate member/index access and decay paths.

struct Buffer {
  int head;
  int values[3];
};

struct Wrapper {
  struct Buffer inner;
  int tail;
};

int sum3(int *ptr) {
  return ptr[0] + ptr[1] + ptr[2];
}

int run(void) {
  struct Wrapper w = {{5, {7, 11, 13}}, 17};
  struct Wrapper *pw = &w;
  int first = w.inner.head;
  int second = pw->inner.values[1];
  int third = w.inner.values[2];
  int decay_dot = sum3(w.inner.values);
  int decay_arrow = sum3(pw->inner.values);
  return first + second + third + decay_dot + decay_arrow + pw->tail;
}

int main(void) {
  return run() == 84 ? 0 : 1;
}
