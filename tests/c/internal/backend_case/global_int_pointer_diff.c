int g_words[2];

int main(void) {
  return (&g_words[1] - &g_words[0]) == 1;
}
