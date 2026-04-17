// Parse-only regression: reserve alternative operator spelling `bitand` as
// the bitwise-and token without breaking ordinary expressions or overloaded
// `operator bitand` declarations.
// RUN: %c4cll --parse-only %s

struct keyword_bitand_box {
    int value;
};

keyword_bitand_box operator bitand(keyword_bitand_box lhs,
                                   keyword_bitand_box rhs) {
    return {lhs.value & rhs.value};
}

int main() {
    int left = 6;
    int right = 3;
    return ((left bitand right) == 2) ? 0 : 1;
}
