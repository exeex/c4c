// Basic destructor declaration and implicit invocation test.
// Tests: ~ClassName() destructor declaration in struct body,
//        implicit destructor call when variable goes out of scope.

int dtor_count = 0;

struct Widget {
    int id;

    Widget(int i) {
        id = i;
    }

    ~Widget() {
        dtor_count = dtor_count + id;
    }
};

int test_single_scope() {
    dtor_count = 0;
    {
        Widget w(10);
        if (w.id != 10) return 1;
    }
    // w goes out of scope, destructor adds 10
    if (dtor_count != 10) return 2;
    return 0;
}

int test_multiple_objects() {
    dtor_count = 0;
    {
        Widget a(1);
        Widget b(2);
        Widget c(3);
    }
    // destructors run in reverse order: c(3) + b(2) + a(1) = 6
    if (dtor_count != 6) return 3;
    return 0;
}

int test_nested_scope() {
    dtor_count = 0;
    {
        Widget outer(100);
        {
            Widget inner(5);
        }
        // inner destroyed here, dtor_count = 5
        if (dtor_count != 5) return 4;
    }
    // outer destroyed here, dtor_count = 5 + 100 = 105
    if (dtor_count != 105) return 5;
    return 0;
}

int main() {
    int r;
    r = test_single_scope();
    if (r != 0) return r;
    r = test_multiple_objects();
    if (r != 0) return r;
    r = test_nested_scope();
    if (r != 0) return r;
    return 0;
}
