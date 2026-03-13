// Expected: compile fail (cast to incomplete struct type)
struct S;

int main() {
  int x = 0;
  return (struct S)x;
}
