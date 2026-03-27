// Parse-only regression: type-like record members should keep their dispatch
// ordering stable while `try_parse_record_member_dispatch()` lifts that branch
// cluster into a focused helper.
// RUN: %c4cll --parse-only %s

class RecordMemberTypeDispatch {
public:
    using Count = int;

    struct Inner {
        Count payload;
    };

    enum Kind { Zero, One };
    typedef Inner Alias;

    Kind kind;
    Alias child;
    Count total() const { return child.payload + kind; }
};

int main() {
    return 0;
}
