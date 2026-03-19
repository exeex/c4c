// Test: comparison operator overloads: operator<, operator>, operator<=, operator>=

extern "C" int printf(const char* fmt, ...);

struct Val {
    int x;
    Val(int v) : x(v) {}

    bool operator<(Val other) const { return x < other.x; }
    bool operator>(Val other) const { return x > other.x; }
    bool operator<=(Val other) const { return x <= other.x; }
    bool operator>=(Val other) const { return x >= other.x; }
    bool operator==(Val other) const { return x == other.x; }
    bool operator!=(Val other) const { return x != other.x; }
};

int main() {
    Val a(3);
    Val b(5);
    Val c(3);

    // operator<
    if (!(a < b)) return 1;
    if (b < a) return 2;
    if (a < c) return 3;  // equal, not less

    // operator>
    if (a > b) return 4;
    if (!(b > a)) return 5;
    if (a > c) return 6;  // equal, not greater

    // operator<=
    if (!(a <= b)) return 7;
    if (b <= a) return 8;
    if (!(a <= c)) return 9;  // equal, should be <=

    // operator>=
    if (a >= b) return 10;
    if (!(b >= a)) return 11;
    if (!(a >= c)) return 12;  // equal, should be >=

    // combined with == and !=
    if (!(a == c)) return 13;
    if (a != c) return 14;
    if (a == b) return 15;
    if (!(a != b)) return 16;

    printf("PASS\n");
    return 0;
}
