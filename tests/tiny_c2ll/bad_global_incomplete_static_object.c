// Expected: compile fail (file-scope static object has incomplete type)
struct S;
static struct S g;

int main(void) {
  return 0;
}
