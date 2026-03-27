// HIR regression: template struct instantiation should keep inherited template
// base resolution and method binding concrete through the instantiated method
// body.

template<typename T>
struct Base {
  T value;
};

template<typename T>
struct Wrap : Base<T> {
  T extra;

  T sum() const {
    return this->value + extra;
  }
};

int main() {
  Wrap<int> w;
  w.value = 10;
  w.extra = 32;
  return w.sum() - 42;
}
