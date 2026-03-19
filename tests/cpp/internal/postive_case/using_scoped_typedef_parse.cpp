// Parse test: aliasing a qualified nested typedef with using.
struct Traits {
  typedef int value_type;
};

using Alias = Traits::value_type;

int main() {
  Alias value = 9;
  return value - 9;
}
