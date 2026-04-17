// Parse-only regression: reserve alternative operator spelling `and` as the
// logical-and token without breaking ordinary expressions or overloaded
// `operator and` declarations.
// RUN: %c4cll --parse-only %s

struct keyword_and_box {
    bool value;
};

bool operator and(keyword_and_box lhs, keyword_and_box rhs) {
    return lhs.value && rhs.value;
}

int main() {
    keyword_and_box t{true};
    keyword_and_box f{false};
    return ((t and t) && !(t and f)) ? 0 : 1;
}
