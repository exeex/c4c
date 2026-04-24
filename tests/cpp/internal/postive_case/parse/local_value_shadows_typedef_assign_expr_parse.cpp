// Parse-only regression: when a local value shadows a typedef name of the same
// spelling, later assignment statements must stay on the expression path.
// RUN: %c4cll --parse-only %s

typedef int result_t;

int adjust(int x) {
    result_t result_t = x;
    result_t = x + 1;
    result_t(result_t);
    return result_t;
}

int main() {
    return adjust(4) == 5 ? 0 : 1;
}
