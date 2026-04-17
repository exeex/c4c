// Parse-only regression: qualified alias-template and dependent qualified
// type-argument heads should stay on the lightweight template type-arg path.
// RUN: %c4cll --parse-only %s

namespace ns {
template <typename T>
struct identity {
  using type = T;
};

template <typename T>
using identity_t = typename identity<T>::type;

template <typename... Ts>
struct pack {};
}  // namespace ns

template <typename T>
struct holder {
  using type = ns::identity_t<T>;
};

template <typename T, typename... Args>
using rebound_t =
    ns::pack<ns::identity_t<T>, typename holder<Args>::type...>;

rebound_t<int, long, char> make_value();

int main() {
  make_value();
  return 0;
}
