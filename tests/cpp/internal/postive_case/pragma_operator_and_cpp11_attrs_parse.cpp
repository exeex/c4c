// Test _Pragma() with macro arguments, [[deprecated]] attribute on structs,
// typename keyword, and unresolved template argument skipping.
// RUN: parse-only

// _Pragma with macro-expanded argument
#define STRINGIFY0(x) #x
#define STRINGIFY1(x) STRINGIFY0(GCC diagnostic ignored x)
#define STRINGIFY2(x) STRINGIFY1(#x)
#define MY_DISABLE_WARNING(w) \
    _Pragma("GCC diagnostic push") \
    _Pragma(STRINGIFY2(w))
#define MY_RESTORE_WARNING() \
    _Pragma("GCC diagnostic pop")

MY_DISABLE_WARNING(-Wdeprecated-declarations)
MY_RESTORE_WARNING()

// [[deprecated]] attribute on struct (C++11 attributes)
template<typename T>
struct [[deprecated]] OldVec {
    T* data;
    int size;
};

// typename keyword in dependent type context
template<typename Container>
struct Wrapper {
    typedef typename Container::value_type value_type;
};

// Unresolved template argument skipping
template<typename T>
struct Box {
    T val;
};

template<typename T1, typename T2>
bool operator==(const Box<T1>& a, const Box<T2>& b) {
    return true;
}

template<typename T1, typename T2>
bool operator<=(const Box<T1>& a, const Box<T2>& b) {
    return true;
}

int main() { return 0; }
