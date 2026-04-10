// Parse-only regression: alias-template application must accept empty type
// packs in default template arguments instead of leaving the opening `<`
// unconsumed and breaking the enclosing template parameter list.
// RUN: %c4cll --parse-only %s

namespace ns {

template <typename...>
using void_t = void;

template <typename, typename = ns::void_t<>>
struct has_equality {
    static constexpr int value = 1;
};

}  // namespace ns

int main() {
    return ns::has_equality<int>::value;
}
