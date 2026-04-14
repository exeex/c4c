char g_text[] = "hi";
char *gp = g_text;

int main(void) {
  gp[1] = 'o';
  return g_text[1];
}
