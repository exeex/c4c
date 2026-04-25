// HIR regression: namespace-qualified module function/global references should
// preserve declaration lookup parity while the structured mirror is active.

namespace api {

int value = 7;

int bump(int x) {
  return x + value;
}

}  // namespace api

int main() {
  int local = api::value;
  return api::bump(local);
}
