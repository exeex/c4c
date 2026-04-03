// Parse-only regression: template-template parameter heads and qualified
// template-ids followed by `::` should stay parseable inside record members.
// RUN: %c4cll --parse-only %s

template <typename T>
struct wrap {
  using type = T;
};

template <template <typename> typename TT, typename T>
struct holder {
  using value_type = typename TT<T>::type;

  value_type value;
};

int main() {
  holder<wrap, int> h;
  return h.value;
}
