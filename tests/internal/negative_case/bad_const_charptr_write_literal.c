// Expected: compile fail (writing through pointer to string literal)
int main() {
  const char *s = "hello";
  s[0] = 'H';
  return 0;
}
