// Runtime regression: unqualified member lookup inside an out-of-class method
// should find inherited non-static base fields through implicit this-> lookup.
struct Base {
    int value;
};

struct Derived : Base {
    int read() const;
    void write(int next);
};

int Derived::read() const {
    return value;
}

void Derived::write(int next) {
    value = next;
}

int main() {
    Derived d{};
    d.value = 5;
    if (d.read() != 5) return 1;
    d.write(9);
    return d.value == 9 ? 0 : 2;
}
