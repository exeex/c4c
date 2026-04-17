// Parse-only regression: reserve alternative operator spelling `not_eq` as
// the inequality token without breaking ordinary `a != b` parsing or
// overloaded `operator not_eq` declarations.
// RUN: %c4cll --parse-only %s

struct keyword_not_eq_box {
    int value;
};

bool operator not_eq(keyword_not_eq_box lhs, keyword_not_eq_box rhs) {
    return lhs.value != rhs.value;
}

int main() {
    int one = 1;
    int two = 2;
    return (one not_eq two && !(one not_eq one)) ? 0 : 1;
}
