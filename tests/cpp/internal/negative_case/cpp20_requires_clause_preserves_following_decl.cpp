// Negative test: a template requires-clause must stop at the end of its
// constraint-expression so the following declaration is still parsed.
template<typename T>
requires true
static_assert(false, "requires-clause should not swallow this declaration");

int main() {
  return 0;
}
