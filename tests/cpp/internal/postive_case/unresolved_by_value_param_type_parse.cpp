// Parse-only regression: unresolved C++ parameter types used by value should
// be treated as type spellings, not K&R-style parameter names.
// RUN: %c4cll --parse-only %s

void accept(Value other);

int main() {
    return 0;
}
