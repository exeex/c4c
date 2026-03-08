// Expected: compile fail (passing const-qualified pointer to mutable required parameter)
int sink(char *p, ...);

int main(void) {
  const char *s = "abc";
  return sink(s, 123);
}
