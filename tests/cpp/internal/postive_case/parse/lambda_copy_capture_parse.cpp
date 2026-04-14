// Parse-only regression: minimal C++ lambda with copy-default capture.
// RUN: %c4cll --parse-only %s

int main() {
    [=] {};
    return 0;
}
