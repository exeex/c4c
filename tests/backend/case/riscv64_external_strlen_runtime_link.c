extern unsigned long strlen(const char *);

int main(void) {
  const char *text = "hello";
  return (int)strlen(text) - 5;
}
