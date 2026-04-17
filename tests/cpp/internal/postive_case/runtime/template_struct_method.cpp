// Test: template struct member functions (methods)
// Phase 5 slice 19

// Non-template struct with method
struct Counter {
  int value;

  int get() const {
    return value;
  }

  int add(int x) {
    return value + x;
  }
};

// Template struct with method
template<typename T>
struct Box {
  T item;

  T get() const {
    return item;
  }

  T add(T x) {
    return item + x;
  }
};

int main() {
  // Non-template struct method calls
  Counter c{10};
  if (c.get() != 10) return 1;
  if (c.add(5) != 15) return 2;

  // Template struct method calls with int
  Box<int> bi{42};
  if (bi.get() != 42) return 3;
  if (bi.add(8) != 50) return 4;

  // Template struct method calls with long
  Box<long> bl{100};
  if (bl.get() != 100) return 5;
  if (bl.add(200) != 300) return 6;

  return 0;
}
