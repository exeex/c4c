// Parse-only regression: direct qualified typedefs and dependent `typename`
// spellings should consume the same qualified-type token path.
// RUN: %c4cll --parse-only %s

namespace api {

struct traits {
    typedef int value_type;
};

template <typename T>
struct holder {
    typedef T type;
};

} // namespace api

template <typename T>
struct consumer {
    ::api::traits::value_type direct;
    typename ::api::holder<T>::type dependent;
};

template <typename T>
typename ::api::holder<T>::type project(T value) {
    typename ::api::holder<T>::type out = value;
    return out;
}

int main() {
    consumer<int> c{};
    c.direct = 1;
    c.dependent = project(2);
    return c.direct + c.dependent - 3;
}
