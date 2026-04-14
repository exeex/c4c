// Parse-only regression: qualified variable template comparisons in expression
// context should not trigger the functional-cast probe.
// RUN: %c4cll --parse-only %s

namespace ns {

template <typename T>
constexpr bool is_signed_v = false;

template <>
constexpr bool is_signed_v<int> = true;

}  // namespace ns

template <typename T, typename U>
int compare_signedness() {
    if constexpr (ns::is_signed_v<T> == ns::is_signed_v<U>)
        return 1;
    return 0;
}

int main() {
    if (compare_signedness<int, int>() != 1)
        return 1;
    if (compare_signedness<int, unsigned>() != 0)
        return 2;
    return 0;
}
