// Template conversion operator — parse-only test
// Exercises: template<class T> operator T*() const;
// This is the pattern used in EASTL/EABase nullptr.h

struct NullPtr {
    template<class T>
    operator T*() const { return 0; }
};

// Template method (non-conversion) inside class body
struct Container {
    int data;

    template<class T>
    T get() const { return (T)data; }
};

int main() {
    return 0;
}
