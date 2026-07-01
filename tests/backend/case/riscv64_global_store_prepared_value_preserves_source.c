int g_sink = 1;

int main(void) {
  int seed = 6;
  int delta = 4;
  int stored = seed + delta;

  g_sink = stored;

  if (g_sink != 10)
    return 1;
  return g_sink == stored ? 0 : 2;
}
