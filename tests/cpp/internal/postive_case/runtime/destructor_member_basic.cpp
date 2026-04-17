// Destructor calls for struct members that are themselves structs with destructors.
// Tests: member destructors called after parent destructor body, in reverse field order.
// Also tests: implicit member dtor calls for structs without explicit dtor.

int log_idx = 0;
int log_buf[32];

void log_event(int code) {
    log_buf[log_idx] = code;
    log_idx = log_idx + 1;
}

struct Inner {
    int id;

    Inner(int i) : id(i) {}

    ~Inner() {
        log_event(id);
    }
};

// Outer has a user-defined destructor and two Inner members.
// Expected destruction order: Outer body (999), then b (20), then a (10) (reverse field order).
struct Outer {
    Inner a;
    Inner b;

    Outer(int x, int y) : a(x), b(y) {}

    ~Outer() {
        log_event(999);
    }
};

int test_member_dtor_with_parent_dtor() {
    log_idx = 0;
    {
        Outer o(10, 20);
    }
    // Expected log: 999 (outer body), 20 (b dtor), 10 (a dtor)
    if (log_idx != 3) return 1;
    if (log_buf[0] != 999) return 2;
    if (log_buf[1] != 20) return 3;
    if (log_buf[2] != 10) return 4;
    return 0;
}

// Wrapper has NO user-defined destructor, but has an Inner member.
// Inner's destructor should still be called when Wrapper goes out of scope.
struct Wrapper {
    Inner item;

    Wrapper(int v) : item(v) {}
};

int test_member_dtor_without_parent_dtor() {
    log_idx = 0;
    {
        Wrapper w(42);
    }
    // Expected log: 42 (item dtor)
    if (log_idx != 1) return 11;
    if (log_buf[0] != 42) return 12;
    return 0;
}

// Nested: Outer has member with dtor, member itself has member with dtor.
struct Middle {
    Inner core;

    Middle(int v) : core(v) {}

    ~Middle() {
        log_event(500);
    }
};

struct Top {
    Middle m;
    Inner extra;

    Top(int a, int b) : m(a), extra(b) {}

    ~Top() {
        log_event(900);
    }
};

int test_nested_member_dtor() {
    log_idx = 0;
    {
        Top t(1, 2);
    }
    // Expected: 900 (Top body), 2 (extra dtor), 500 (Middle body), 1 (core dtor)
    if (log_idx != 4) return 21;
    if (log_buf[0] != 900) return 22;
    if (log_buf[1] != 2) return 23;
    if (log_buf[2] != 500) return 24;
    if (log_buf[3] != 1) return 25;
    return 0;
}

int main() {
    int r;
    r = test_member_dtor_with_parent_dtor();
    if (r != 0) return r;
    r = test_member_dtor_without_parent_dtor();
    if (r != 0) return r;
    r = test_nested_member_dtor();
    if (r != 0) return r;
    return 0;
}
