// Expected: compile fail (_Static_assert requires integer constant expression)
int x = 1;
_Static_assert(x, "not constant");

int main() {
  return 0;
}
