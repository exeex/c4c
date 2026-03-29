// Parse-only regression: reserve alternative operator spelling `compl` as the
// bitwise-not token without breaking unary expressions or overloaded
// `operator compl` declarations.
// RUN: %c4cll --parse-only %s

struct keyword_compl_box {
    int value;
};

keyword_compl_box operator compl(keyword_compl_box value) {
    return {~value.value};
}

int main() {
    keyword_compl_box value{12};
    return ((compl value).value == (~12) && compl 3 == ~3) ? 0 : 1;
}
