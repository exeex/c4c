char g_text[] = "hi";
char *gp = g_text;
char **ggp = &gp;

int main(void) {
  (*ggp)[1] = 'o';
  return g_text[1];
}
