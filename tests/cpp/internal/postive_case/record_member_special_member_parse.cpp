// Parse-only regression: record-body constructors and destructors should stay
// recognizable while special-member handling is extracted out of the
// struct-body loop.
// RUN: %c4cll --parse-only %s

struct RecordSpecialMembers {
    int value;

    RecordSpecialMembers(int v) : value(v) {}
    RecordSpecialMembers() = default;
    RecordSpecialMembers(const RecordSpecialMembers&) = delete;
    ~RecordSpecialMembers() = default;

    int tail;
};

int main() {
    return 0;
}
