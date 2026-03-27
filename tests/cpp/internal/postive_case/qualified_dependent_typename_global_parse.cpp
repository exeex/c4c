// Parse-only: qualified/dependent type spelling should consume the same token
// path for global-qualified typename forms and qualified base types.
// RUN: %c4cll --parse-only %s

namespace api {

template <typename T>
struct holder {
    typedef T type;
};

} // namespace api

template <typename T>
struct consumer {
    typedef typename ::api::holder<T>::type value_type;
    value_type value;
};

template <typename T>
typename ::api::holder<T>::type lift(T value) {
    typename ::api::holder<T>::type out = value;
    return out;
}

int main() {
    ::api::holder<int> wrapped{};
    consumer<int> c{};
    c.value = lift(1);
    (void)wrapped;
    return c.value;
}
