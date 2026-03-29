// Parse-only regression: malformed local using-alias declarations should
// recover at the following declaration boundary instead of silently erasing it.

void f() {
  using Alias = int
  int kept = 1;
}
