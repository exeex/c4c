// Expected: compile fail (local typedef redefined to different type)
int main(void) {
  typedef int T;
  typedef char T;

  typedef short U, V;
  typedef short U, long V;
  return 0;
}
