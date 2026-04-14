// Parse-only regression: record-member entry should keep attribute skipping,
// prelude handling, and trailing end-of-body detection stable while
// `try_parse_record_member()` collapses into a pure coordinator.
// RUN: %c4cll --parse-only %s

class RecordMemberEntry {
public:
    __attribute__((aligned(8))) int first;

private:
    friend void touch(RecordMemberEntry&);
    static_assert(sizeof(int) >= 4, "int width");
    __attribute__((aligned(16))) int second;

public:
};

int main() {
    return 0;
}
