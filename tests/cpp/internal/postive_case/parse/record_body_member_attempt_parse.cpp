// Parse-only regression: the record-body loop should keep per-member attempt
// and malformed-member recovery staging isolated while `parse_record_body()`
// lifts that path into a focused helper.
// RUN: %c4cll --parse-only %s

class RecordBodyMemberAttempt {
public:
    int before;
    int broken( { return 0; };

    static_assert(sizeof(int) >= 4, "int width");

    template <typename T = int>
    struct Holder {
        T value;
    };

    using Alias = Holder<>;
    enum Kind { Zero, One } kind;

    RecordBodyMemberAttempt() = default;
    int total() const { return before + after + kind + payload.value; }
    int after;
    Alias payload;
};

int main() {
    return 0;
}
