// Parse-only regression: when a local value shadows a visible record tag of
// the same spelling, later assignment statements must stay on the expression
// path instead of being misrouted through local-declaration parsing.
// RUN: %c4cll --parse-only %s

struct __result {};

int adjust(int x) {
    int __result = x;
    __result = x + 1;
    return __result;
}

int main() {
    return adjust(4) == 5 ? 0 : 1;
}
