// Parse-only regression: C++17/C++20 `if (init; condition)` should keep the
// init declaration scoped while parsing the trailing condition expression.
// RUN: %c4cll --parse-only %s

struct ordering {
    int value;

    ordering(int v) : value(v) {}

    friend bool operator!=(ordering lhs, int rhs) {
        return lhs.value != rhs;
    }
};

ordering compare(int lhs, int rhs) {
    if (auto diff = ordering(lhs - rhs); diff != 0) {
        return diff;
    }
    return ordering(0);
}

int main() {
    return compare(4, 1).value;
}
