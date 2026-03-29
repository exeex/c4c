// Parse-only regression: minimal C++ lambda with an empty capture default.
// RUN: %c4cll --parse-only %s

int main() {
    [] {};
    return 0;
}
