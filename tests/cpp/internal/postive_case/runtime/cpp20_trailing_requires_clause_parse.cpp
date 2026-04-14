// Reduced from libstdc++ C++20 headers: a function declarator can carry a
// trailing requires-clause after its parameter list and still keep the body
// attached for instantiation.

template<typename T>
struct trait {
    static constexpr bool value = true;
};

template<typename T>
int accept(T value) requires (trait<T>::value) {
    return value + 4;
}

int main() {
    return accept(3) - 7;
}
