// Parse-only regression: mixed record-body member dispatch should remain
// stable while `parse_struct_or_union()` flattens toward a helper-driven
// dispatcher.
// RUN: %c4cll --parse-only %s

class RecordDispatch {
public:
    template <typename T = int>
    struct Holder {
        T value;
    };

    friend bool operator==(const RecordDispatch&, const RecordDispatch&) {
        return true;
    }

    static_assert(sizeof(int) >= 4, "int width");
    using Alias = Holder<>;
    enum Kind { Zero, One } kind;

    RecordDispatch() = default;
    ~RecordDispatch() = default;

    int first, second;
    unsigned bits : 3;
    static constexpr int seed = 7;

    int total() const { return first + second + bits + seed; }

private:
    Holder<> payload;
};

int main() {
    return 0;
}
