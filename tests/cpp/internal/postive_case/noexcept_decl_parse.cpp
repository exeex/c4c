// Parse test: declarations should tolerate noexcept(bool-constant) suffixes.
extern int sample(int x) noexcept(true);

int main() {
  return 0;
}
