struct Inner {
  int arr[2];
};

struct Outer {
  struct Inner *inner;
};

int get_second(struct Outer *o) {
  return o->inner->arr[1];
}

int main(void) {
  struct Inner inner = {{4, 8}};
  struct Outer outer = {&inner};
  return get_second(&outer);
}
