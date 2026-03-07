// Expected: compile fail (discard const qualifier via intermediate pointer)
int main() {
  const char *s = "hello";
  char *p = s;
  return p[0];
}
