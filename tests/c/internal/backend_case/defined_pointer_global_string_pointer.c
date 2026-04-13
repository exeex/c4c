char g_text[] = "hi";
char *gp = g_text;
char **ggp = &gp;

int main(void) { return (*ggp)[1]; }
