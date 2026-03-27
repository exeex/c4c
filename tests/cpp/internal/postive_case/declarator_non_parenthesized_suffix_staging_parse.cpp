// Parse-only: non-parenthesized declarators should keep grouped-vs-plain
// suffix staging stable while the coordinator extracts shared dispatch.
// RUN: %c4cll --parse-only %s

typedef int Value;

struct Holder {
    static Value rows[2][3];
    Value field;
    Value method(Value (grouped_param), Value plain_param[2]);
};

Value Holder::rows[2][3] = {{1, 2, 3}, {4, 5, 6}};

Value (grouped_scalar), plain_slots[4], (grouped_rows[2])[3];

Value Holder::method(Value (grouped_param), Value plain_param[2]) {
    return grouped_param + plain_param[0];
}

Value consume_mix(Value (grouped_param), Value plain_param[2],
                  Value (grouped_matrix[2])[3]);

int main() {
    plain_slots[0] = Holder::rows[1][2];
    grouped_rows[0][0] = grouped_scalar;
    return Holder{}.method(grouped_scalar, plain_slots) + grouped_rows[0][0];
}
