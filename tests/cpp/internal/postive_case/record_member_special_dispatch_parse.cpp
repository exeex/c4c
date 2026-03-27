// Parse-only regression: record-body constructor/destructor dispatch should
// stay isolated from the type-like and method/field branches while the
// special-member staging moves behind a helper.
// RUN: %c4cll --parse-only %s

class RecordSpecialDispatch {
public:
    using Value = int;
    enum Kind { Primary, Secondary } kind;

    RecordSpecialDispatch() = default;
    RecordSpecialDispatch(Value v) : kind(v ? Primary : Secondary), value(v) {}
    ~RecordSpecialDispatch() = default;

    Value value;
    Value total() const { return value + static_cast<Value>(kind); }
};

int main() {
    return 0;
}
