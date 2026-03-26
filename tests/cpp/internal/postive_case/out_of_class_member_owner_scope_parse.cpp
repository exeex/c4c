// Parse-only: out-of-class member definitions should re-enter the owner's
// template scope (EnclosingClass) and make member typedefs/nested types
// visible in parameter lists and function bodies.

// Case 1: simple member typedef visible in out-of-class definition
template <typename T>
struct Container {
    typedef T value_type;
    typedef unsigned long size_type;
    T* data_;
    size_type size_;
    void resize(size_type n);
    value_type get(size_type idx) const;
};

template <typename T>
void Container<T>::resize(size_type n) {
    // size_type must resolve as a type (from the owner struct's typedefs)
    size_type old = size_;
    (void)old;
    (void)n;
}

template <typename T>
typename Container<T>::value_type Container<T>::get(size_type idx) const {
    value_type val = data_[idx];
    return val;
}

// Case 2: nested template member with both enclosing and member template params
template <typename T>
struct Pair {
    typedef T first_type;
    T first;

    template <typename U>
    void assign(first_type a, U b);
};

template <typename T>
template <typename U>
void Pair<T>::assign(first_type a, U b) {
    first_type local = a;
    (void)local;
    (void)b;
}

// Case 3: destructor out-of-class (tests owner scope for special members)
template <typename T>
struct Holder {
    typedef T held_type;
    T* ptr;
    ~Holder();
};

template <typename T>
Holder<T>::~Holder() {
    held_type* p = ptr;
    (void)p;
}

int main() {
    return 0;
}
