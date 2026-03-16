// Test: Phase C slice 2 — CastExpr, IndexExpr, and CallExpr fn_ptr resolution
// Verifies that resolve_callee_fn_ptr_sig handles:
//   1. Cast to function pointer type: ((fn_type)expr)(args)
//   2. Array indexing of fn_ptr arrays: fn_arr[i](args)
//   3. Call returning fn_ptr: get_fn()(args)
//
// Each test passes an int where fn expects short, verifying coercion occurs.

typedef long (*widen_fn)(short);

long widen_add(short x) { return (long)x + 1000; }
long widen_sub(short x) { return (long)x - 1000; }

// 1. Cast expression: cast a void* to fn_ptr typedef and call with int arg.
long call_via_cast(void *p, int v) {
    return ((widen_fn)p)(v);   // v (int) must be coerced to short
}

// 2. Index expression: array of fn_ptrs indexed and called with int arg.
long call_via_index(int v) {
    widen_fn arr[2];
    arr[0] = widen_add;
    arr[1] = widen_sub;
    return arr[0](v) + arr[1](v);  // v (int) must be coerced to short
}

// 3. Call expression returning fn_ptr, then called with int arg.
widen_fn get_fn(int sel) {
    if (sel) return widen_add;
    return widen_sub;
}

long call_via_returned_fn_ptr(int sel, int v) {
    return get_fn(sel)(v);   // v (int) must be coerced to short
}

int main(void) {
    // Cast: widen_add((short)42) => 42 + 1000 = 1042
    long r1 = call_via_cast((void*)widen_add, 42);
    if (r1 != 1042) return 1;

    // Index: widen_add((short)10) + widen_sub((short)10) = 1010 + (-990) = 20
    long r2 = call_via_index(10);
    if (r2 != 20) return 2;

    // Call returning fn_ptr
    long r3 = call_via_returned_fn_ptr(1, 5);
    if (r3 != 1005) return 3;

    long r4 = call_via_returned_fn_ptr(0, 5);
    if (r4 != -995) return 4;

    return 0;
}
