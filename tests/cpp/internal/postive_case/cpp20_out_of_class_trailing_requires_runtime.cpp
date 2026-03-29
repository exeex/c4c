// Reduced from constrained out-of-class member definitions: constructors and
// conversion operators should keep their trailing requires-clauses attached so
// the instantiated member bodies stay bound to the owning record.

template<typename T>
struct enabled {
    static constexpr bool value = true;
};

template<typename T>
struct holder {
    T value;

    holder(T v) requires (enabled<T>::value);
    operator bool() const requires (enabled<T>::value);
};

template<typename T>
holder<T>::holder(T v) requires (enabled<T>::value) : value(v) {}

template<typename T>
holder<T>::operator bool() const requires (enabled<T>::value) {
    return value != 0;
}

int main() {
    holder<int> h{7};
    return h ? 0 : 1;
}
