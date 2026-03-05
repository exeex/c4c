// Expected: compile fail (struct tag redefined with different layout)
struct S { int a; };
struct S { char b; };

int main() {
  return 0;
}
