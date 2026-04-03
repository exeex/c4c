// Parse-only regression: record-member typedefs may use Clang/GCC transform
// type builtins with parenthesized type operands.
// RUN: %c4cll --parse-only %s

enum class Flag : unsigned int {
    On = 1u,
};

template <typename T>
struct underlying_holder {
    typedef __underlying_type(T) type;
    type value;
};

int main() {
    underlying_holder<Flag> holder{};
    return static_cast<int>(holder.value);
}
