// Parse-only regression: regular in-record methods and fields should remain
// parseable while the default record-member branch is extracted out of the
// struct-body loop.
// RUN: %c4cll --parse-only %s

struct RecordMethodField {
    static constexpr int seed = 7;
    unsigned bits : 3;
    int first, second;

    RecordMethodField(int value) : bits(value), first(value), second(value + seed) {}

    int get() const { return first + second + bits; }
    explicit operator bool() const { return bits != 0; }
};

int main() {
    return 0;
}
