// Parse-only regression: reserve alternative operator spelling `not` as the
// logical-not token without breaking ordinary unary expressions or overloaded
// `operator not` declarations.
// RUN: %c4cll --parse-only %s

struct keyword_not_box {
    bool value;
};

bool operator not(keyword_not_box value) {
    return !value.value;
}

int main() {
    keyword_not_box t{true};
    keyword_not_box f{false};
    return (not f && !not t) ? 0 : 1;
}
