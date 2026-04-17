// Constructor initializer list: ClassName(params) : member(init), ... { body }
// Tests: basic member initialization, multiple members, expression initializers,
//        initializer list with empty body, mixed init-list + body assignments.
struct Point {
    int x;
    int y;

    Point(int a, int b) : x(a), y(b) {}

    int sum() const {
        return x + y;
    }
};

struct Rect {
    int left;
    int top;
    int width;
    int height;

    Rect(int l, int t, int w, int h) : left(l), top(t), width(w), height(h) {}

    int area() const {
        return width * height;
    }
};

struct Counter {
    int val;
    int step;

    // Initializer list with expression
    Counter(int start, int s) : val(start * 2), step(s + 1) {}
};

struct Mixed {
    int a;
    int b;
    int c;

    // Partial init list + body assignment
    Mixed(int x, int y) : a(x), b(y) {
        c = a + b;
    }
};

int main() {
    // Basic two-member init
    Point p(3, 4);
    if (p.x != 3) return 1;
    if (p.y != 4) return 2;
    if (p.sum() != 7) return 3;

    // Four-member init
    Rect r(10, 20, 30, 40);
    if (r.left != 10) return 4;
    if (r.top != 20) return 5;
    if (r.area() != 1200) return 6;

    // Expression in initializer
    Counter c(5, 2);
    if (c.val != 10) return 7;   // 5 * 2
    if (c.step != 3) return 8;   // 2 + 1

    // Mixed init list + body
    Mixed m(7, 3);
    if (m.a != 7) return 9;
    if (m.b != 3) return 10;
    if (m.c != 10) return 11;  // 7 + 3

    return 0;
}
