// Parse-only regression: minimal C++ lambda with an explicit empty parameter list.
// RUN: %c4cll --parse-only %s

int main() {
    []() {};
    return 0;
}
