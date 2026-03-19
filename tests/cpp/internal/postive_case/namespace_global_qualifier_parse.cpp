// Parse test: references inside a namespace can explicitly start from the
// global namespace via ::name.
int some_global_var = 43;

namespace xx {
int read_global() {
  return ::some_global_var;
}
}

int main() {
  return xx::read_global() - 43;
}
