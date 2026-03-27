// Parse-only regression: access labels, friend/static_assert preludes, and
// ordinary members should interleave cleanly while `try_parse_record_member()`
// collapses toward a thin prelude-or-dispatch coordinator.
// RUN: %c4cll --parse-only %s

class RecordMemberMixedPrelude {
public:
    friend struct RecordFriend;
    int first;

private:
    static_assert(sizeof(int) >= 4, "int width");
    int second;

public:
    friend bool operator==(const RecordMemberMixedPrelude&,
                           const RecordMemberMixedPrelude&) {
        return true;
    }

    int total() const { return first + second; }

protected:
    static_assert(sizeof(long long) >= sizeof(int), "width check");
    int third;
};

int main() {
    return 0;
}
