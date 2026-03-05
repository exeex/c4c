// Expected: compile fail (discard const qualifier from const char*)
int main() {
  const char *s = "hello";
  char *p = s;
  return p[0];
}
