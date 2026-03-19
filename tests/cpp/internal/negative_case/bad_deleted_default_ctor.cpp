// Negative test: T var; with deleted default constructor should be rejected.
struct NoDefault {
    int value;
    NoDefault() = delete;
    NoDefault(int v) { value = v; }
};

int main() {
    NoDefault x;
    return 0;
}
