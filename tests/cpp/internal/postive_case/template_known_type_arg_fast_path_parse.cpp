// Parse-only regression: simple known type template arguments should still
// parse when the parser takes the lightweight type-argument path.
// RUN: %c4cll --parse-only %s

template <typename T>
struct identity {
  using type = T;
};

template <typename... Ts>
struct pack {};

template <typename T, typename... Args>
struct wrap {
  using nested = pack<T, Args...>;
};

template <typename T>
struct holder {
  using type = identity<T>;
};

template <typename T, typename... Args>
using rebound_t =
    wrap<typename holder<T>::type, identity<Args>...>;

rebound_t<int, long, char> make_value();

int main() {
  make_value();
  return 0;
}
