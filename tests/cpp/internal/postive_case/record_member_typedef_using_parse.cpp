// Parse-only regression: record-body using aliases and typedef members should
// stay registered for later member declarations while the parser body is
// flattened into helper dispatch.
// RUN: %c4cll --parse-only %s

struct RecordAliases {
    using Count = unsigned long;
    typedef Count* CountPtr;
    typedef int Small, *SmallPtr;
    typedef int (*Callback)(Count);

    Count value;
    CountPtr next;
    Small current;
    SmallPtr slot;
    Callback cb;
};

int main() {
    return 0;
}
