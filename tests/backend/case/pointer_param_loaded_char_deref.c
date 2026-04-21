int load_first(const char **s) {
  return **s;
}

int main(void) {
  const char *p = "hi";
  return load_first(&p) - 'h';
}
