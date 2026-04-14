// Parse-only regression: reserve `typename` and `final` as keyword tokens
// without breaking the existing parser compatibility paths that rely on them.
// RUN: %c4cll --parse-only %s

template <typename T>
struct box final {
    using value_type = T;
};

template <typename T>
typename box<T>::value_type project(typename box<T>::value_type value) {
    return value;
}

int main() {
    return project<int>(7) - 7;
}
