struct Counter {
  int value;

  int get() const {
    return value;
  }
};

int main() {
  Counter counter{42};
  int result = counter.get();
  int expect = 42;
  if (result == expect) return 0;
  else return -1;
}
