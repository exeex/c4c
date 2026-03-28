char g_bytes[2];

int main(void) {
  return (&g_bytes[1] - &g_bytes[0]) == 1;
}
