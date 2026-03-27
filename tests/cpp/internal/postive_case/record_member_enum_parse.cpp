// Parse-only regression: record-body enum members should preserve enum type
// association, declarator parsing, and bitfield width capture while the
// struct-body loop is flattened into helper dispatch.
// RUN: %c4cll --parse-only %s

struct RecordEnumMembers {
    enum Named {
        Alpha,
        Beta
    } named, kind : 2;

    enum {
        Hidden,
        Exposed
    } anon, mask : 1;

    int tail;
};

int main() {
    return 0;
}
