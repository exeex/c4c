// Parse-only regression: record-body using aliases and typedef members should
// register through parser-local typedef helpers and remain visible to the
// following member declarations.
// RUN: %c4cll --parse-only %s

struct RecordAliasesDump {
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
