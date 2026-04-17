// Reduced from constrained class-template members: record-member constructors,
// conversion operators, and ordinary methods should keep their trailing
// requires-clauses attached so the instantiated members still have bodies.

template<typename T>
struct enabled {
    static constexpr bool value = true;
};

template<typename T>
struct holder {
    T value;

    holder(T v) requires (enabled<T>::value) : value(v) {}
    operator bool() const requires (enabled<T>::value) { return value != 0; }
    int get() const requires (enabled<T>::value) { return value; }
};

int main() {
    holder<int> h{7};
    return h ? (h.get() - 7) : 1;
}
