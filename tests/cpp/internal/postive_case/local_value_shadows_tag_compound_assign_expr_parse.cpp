// Parse-only regression: when a local value shadows a visible record tag, a
// later compound assignment must stay on the expression path.
// RUN: %c4cll --parse-only %s

struct total {};

int adjust(int x) {
    int total = x;
    total += 3;
    return total;
}

int main() {
    return adjust(4) == 7 ? 0 : 1;
}
