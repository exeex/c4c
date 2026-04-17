// Runtime test: cross-namespace type references as function parameters and return types,
// including nested namespace functions and types.
namespace shapes {
struct Rect {
    int w;
    int h;
};

int area(Rect r) { return r.w * r.h; }
}

namespace utils {
shapes::Rect make_rect(int w, int h) {
    shapes::Rect r;
    r.w = w;
    r.h = h;
    return r;
}

int compute_area(shapes::Rect r) {
    return shapes::area(r);
}
}

// Nested namespace with cross-namespace type usage from outer scope.
namespace outer {
namespace inner {
struct Config {
    int val;
};
int get_val(Config c) { return c.val; }
}
inner::Config make_config(int v) {
    inner::Config c;
    c.val = v;
    return c;
}
}

int main() {
    // Cross-namespace type as return type and parameter
    shapes::Rect r = utils::make_rect(3, 4);
    if (r.w != 3) return 1;
    if (r.h != 4) return 2;
    if (utils::compute_area(r) != 12) return 3;

    // Nested namespace function and type references
    outer::inner::Config c;
    c.val = 42;
    if (outer::inner::get_val(c) != 42) return 4;
    outer::inner::Config c2 = outer::make_config(99);
    if (c2.val != 99) return 5;

    return 0;
}
