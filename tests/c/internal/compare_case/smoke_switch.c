// Compare-mode smoke test: switch, string constants.
extern int printf(const char *, ...);

const char *day_name(int d) {
  switch (d) {
  case 0: return "Sun";
  case 1: return "Mon";
  case 2: return "Tue";
  default: return "?";
  }
}

int main() {
  const char *s = day_name(1);
  return s[0] == 'M' ? 0 : 1;
}
