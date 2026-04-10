// Parse-only regression: record-body template-origin setup plus typedef-backed
// enum underlying types should remain visible while `parser_types_struct.cpp`
// finishes routing enum typedef reads through parser-local helpers.
// RUN: %c4cll --parse-only %s

template <typename T>
struct EnumUnderlyingHolder;

template <>
struct EnumUnderlyingHolder<int> {
    typedef unsigned short Underlying;
    using Self = EnumUnderlyingHolder;

    enum Kind : Underlying {
        Alpha = 1,
        Beta = 2
    } kind;

    Self bounce(Self other) { return other; }
};

int main() {
    EnumUnderlyingHolder<int> holder{};
    return static_cast<int>(holder.kind);
}
