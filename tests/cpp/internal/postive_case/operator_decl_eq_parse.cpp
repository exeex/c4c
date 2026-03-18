// Parse test: operator== and operator!= declarations with int parameter.
struct Counter {
  int val;

  bool operator==(int other) {
    return val == other;
  }

  bool operator!=(int other) {
    return val != other;
  }
};

int main() {
  return 0;
}
