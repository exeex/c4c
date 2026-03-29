// Parse-only regression: reserve alternative operator spelling `or` as the
// logical-or token without breaking ordinary expressions or overloaded
// `operator or` declarations.
// RUN: %c4cll --parse-only %s

struct keyword_or_box {
    bool value;
};

bool operator or(keyword_or_box lhs, keyword_or_box rhs) {
    return lhs.value || rhs.value;
}

int main() {
    keyword_or_box t{true};
    keyword_or_box f{false};
    return ((f or t) && !(f or f)) ? 0 : 1;
}
