// RUN: %c4cll --parse-only %s

namespace eastl {
template <typename T>
using alias_t = T;

template <typename... T>
using variadic_alias_t = int;

struct Box {
    int size() const;
};

template <class C>
auto f(const C& c) -> variadic_alias_t<int, alias_t<int>> {
    return c.size();
}
}  // namespace eastl
