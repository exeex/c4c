// Test: expression-level canonical types for indirect calls
// Phase C: typedef fn_ptr param propagation + struct member fn_ptr resolution

typedef long (*widen_fn)(short);

long widen_add(short x) { return (long)x + 1000; }
long widen_sub(short x) { return (long)x - 1000; }

struct ops {
    widen_fn apply;
    int tag;
};

// Indirect call through struct member function pointer.
// Verifies that the fn_ptr signature is resolved so the short param
// is coerced correctly (not left as default-promoted int).
long call_via_member(struct ops *o, short v) {
    return o->apply(v);
}

// Indirect call through a typedef'd fn_ptr local variable.
long call_via_local(short v) {
    widen_fn f = widen_add;
    return f(v);
}

// Indirect call through ternary-selected fn_ptr variable.
long call_via_ternary(int sel, short v) {
    widen_fn f = sel ? widen_add : widen_sub;
    return f(v);
}

// Typedef'd fn_ptr as function parameter.
long call_via_param(widen_fn f, short v) {
    return f(v);
}

int main(void) {
    struct ops o;
    o.apply = widen_add;
    o.tag = 1;

    // struct member fn_ptr
    long r1 = call_via_member(&o, 42);
    if (r1 != 1042) return 1;

    o.apply = widen_sub;
    long r2 = call_via_member(&o, 42);
    if (r2 != -958) return 2;

    // local typedef'd fn_ptr
    long r3 = call_via_local(10);
    if (r3 != 1010) return 3;

    // ternary fn_ptr
    long r4 = call_via_ternary(1, 10);
    if (r4 != 1010) return 4;

    long r5 = call_via_ternary(0, 10);
    if (r5 != -990) return 5;

    // param fn_ptr
    long r6 = call_via_param(widen_add, 5);
    if (r6 != 1005) return 6;

    return 0;
}
