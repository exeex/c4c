// Regression: C-style casts to references must preserve value category for
// ref-qualified method dispatch.

struct Probe {
  int select() & { return 1; }
  int select() && { return 2; }
};

int main() {
  Probe p;

  if (((Probe&)p).select() != 1) return 1;
  if (((Probe&&)p).select() != 2) return 2;

  return 0;
}
