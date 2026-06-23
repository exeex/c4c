struct Slot {
  int (*fn)(int);
};

int add_three(int x) { return x + 3; }

int main(void) {
  struct Slot slot;
  int (*local_fn)(int);

  slot.fn = add_three;
  local_fn = slot.fn;

  return local_fn(4) - 7;
}
