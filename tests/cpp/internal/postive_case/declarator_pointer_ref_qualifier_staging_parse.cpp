// Parse-only: pointer/reference declarator qualifier staging should remain
// stable across plain declarators and parenthesized function-pointer forms.
// RUN: %c4cll --parse-only %s

typedef int Value;

Value target(Value value) {
    return value + 1;
}

Value *const global_pointer = 0;
Value *const *volatile pointer_chain = 0;
Value (*callback_table[2])(Value);
Value (*volatile selected_callback)(Value) = target;
Value (&callback_ref)(Value) = target;

struct Holder {
    Value *const field_ptr;
    Value (*const field_callback)(Value);
};

Value consume(Value *const *volatile slots,
              Value (*const callback)(Value),
              Value (&ref)(Value)) {
    (void)slots;
    return callback(ref(0));
}

int main() {
    callback_table[0] = target;
    return consume(pointer_chain, selected_callback, callback_ref) == 2 ? 0 : 1;
}
