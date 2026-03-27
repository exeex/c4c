// Parse-only regression: `parse_base_type()` should route dependent
// `typename` forms and global-qualified typedef-like names through one C++
// scoped base-type dispatcher.
// RUN: %c4cll --parse-only %s

namespace api {

template <typename T>
struct holder {
    typedef T type;
};

typedef int count_t;

} // namespace api

template <typename T>
typename ::api::holder<T>::type lift(T value) {
    typename ::api::holder<T>::type local = value;
    ::api::count_t offset = 0;
    return local + offset;
}

int main() {
    return lift(1);
}
