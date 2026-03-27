// Parse-only: grouped declarators should keep identifier-vs-parameter-list
// lookahead and staged array suffix application stable during extraction.
// RUN: %c4cll --parse-only %s

typedef int Value;

Value (plain_value);
Value *(pointer_slots[3]);
Value (matrix_rows[2])[4];

struct Holder {
    Value (member_value);
    Value *(member_slots[2]);
};

Value consume(Value (param_value), Value *(param_slots[3])) {
    (void)param_slots;
    return param_value;
}

int main() {
    pointer_slots[0] = &plain_value;
    return consume(plain_value, pointer_slots);
}
