// Parse-only regression: C++ declaration conditions in `if (...)` should parse
// as a scoped declaration plus condition check.
// RUN: %c4cll --parse-only %s

int adjust(int first, int last) {
    if (const int len = last - first)
        return len - 2;
    return 7;
}

int main() {
    return adjust(1, 3);
}
