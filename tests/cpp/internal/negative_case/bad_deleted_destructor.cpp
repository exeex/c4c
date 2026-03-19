// Negative test: deleted destructor should prevent variable declaration.
// EXPECTED_ERROR: deleted destructor

struct NoDtor {
    int x;
    ~NoDtor() = delete;
};

int main() {
    NoDtor n;  // ERROR: destructor is deleted
    return 0;
}
