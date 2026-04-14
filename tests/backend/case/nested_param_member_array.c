struct Inner {
  int arr[2];
};

struct Outer {
  struct Inner inner;
};

int get_second(struct Outer o) {
  return o.inner.arr[1];
}

int main(void) {
  struct Outer outer = {{{4, 9}}};
  return get_second(outer);
}
