// Test: struct types defined inside a namespace can be used via qualified name.
namespace geo {
struct Point {
    int x;
    int y;
};
}

int main() {
    geo::Point p;
    p.x = 10;
    p.y = 20;
    if (p.x != 10) return 1;
    if (p.y != 20) return 2;
    return 0;
}
