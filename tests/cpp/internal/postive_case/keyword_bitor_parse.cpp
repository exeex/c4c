// Parse-only regression: reserve alternative operator spelling `bitor` as
// the bitwise-or token without breaking ordinary expressions or overloaded
// `operator bitor` declarations.
// RUN: %c4cll --parse-only %s

struct keyword_bitor_box {
    int value;
};

keyword_bitor_box operator bitor(keyword_bitor_box lhs,
                                 keyword_bitor_box rhs) {
    return {lhs.value | rhs.value};
}

int main() {
    int left = 4;
    int right = 1;
    return ((left bitor right) == 5) ? 0 : 1;
}
