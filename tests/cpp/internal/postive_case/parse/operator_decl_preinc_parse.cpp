// Parse test: prefix operator++ and postfix operator++(int).
struct Counter {
  int val;

  Counter& operator++() {
    val++;
    return *this;
  }

  Counter operator++(int dummy) {
    Counter old{val};
    val++;
    return old;
  }
};

int main() {
  return 0;
}
