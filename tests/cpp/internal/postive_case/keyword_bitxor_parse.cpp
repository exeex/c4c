// Parse-only regression: reserve alternative operator spelling `bitxor` as
// the bitwise-xor token without breaking ordinary expressions or overloaded
// `operator bitxor` declarations.
// RUN: %c4cll --parse-only %s

struct keyword_bitxor_box {
    int value;
};

keyword_bitxor_box operator bitxor(keyword_bitxor_box lhs,
                                   keyword_bitxor_box rhs) {
    return {lhs.value ^ rhs.value};
}

int main() {
    int left = 6;
    int right = 3;
    return ((left bitxor right) == 5) ? 0 : 1;
}
