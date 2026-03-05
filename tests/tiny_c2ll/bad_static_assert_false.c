// Expected: compile fail (_Static_assert condition is false)
_Static_assert(0, "must fail");

int main() {
  return 0;
}
