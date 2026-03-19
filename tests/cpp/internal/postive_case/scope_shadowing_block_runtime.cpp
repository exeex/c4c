// Runtime test: nested block scope should shadow outer locals without
// permanently overwriting them.
int main() {
  int value = 3;
  int total = 0;

  total += value;

  {
    int value = 7;
    total += value;

    {
      int value = 11;
      total += value;
    }

    total += value;
  }

  total += value;

  if (total != 31) return 1;
  return 0;
}
