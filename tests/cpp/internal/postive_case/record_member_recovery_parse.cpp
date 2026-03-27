// Parse-only regression: record-body recovery should skip one malformed member
// and keep later member categories parseable while `parse_struct_or_union()`
// moves toward helperized dispatch and recovery.
// RUN: %c4cll --parse-only %s

class RecordRecovery {
public:
    int before;
    int broken( { return 0; };

    static_assert(sizeof(int) >= 4, "int width");

    template <typename T>
    struct Holder {
        T value;
    };

    enum Kind { Zero, One } kind;
    RecordRecovery() = default;
    int total() const { return before + after + kind; }
    int after;
};

int main() {
    return 0;
}
