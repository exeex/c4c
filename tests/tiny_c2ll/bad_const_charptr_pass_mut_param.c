// Expected: compile fail (pass const char* to char* parameter)
int mutate_first(char *p) {
  p[0] = 'X';
  return p[0];
}

int main() {
  const char *s = "hello";
  return mutate_first(s);
}
