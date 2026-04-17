// Parse-only regression: `Trait<...>::value` arguments to alias templates
// should go straight to non-type parsing instead of paying an expensive
// speculative type-id probe first.
// RUN: %c4cll --parse-only %s

template <bool Cond, typename TrueType, typename FalseType>
struct conditional {
  using type = TrueType;
};

template <typename Base, typename Derived>
struct is_base_of {
  static constexpr bool value = false;
};

template <bool Cond, typename TrueType, typename FalseType>
using conditional_t = typename conditional<Cond, TrueType, FalseType>::type;

template <typename Base, typename Derived>
using pick_t = conditional_t<is_base_of<Base, Derived>::value, Base, Derived>;

pick_t<int, long> select_value();

int main() {
  select_value();
  return 0;
}
