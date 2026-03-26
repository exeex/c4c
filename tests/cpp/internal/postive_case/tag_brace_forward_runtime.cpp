// Reduced repro for invoke-style calls that pass a braced temporary as the
// first argument to a templated call.

struct Tag {};

template<typename T>
T&& forward(T& t) { return static_cast<T&&>(t); }

template<typename R>
int impl(Tag, R v) { return v; }

template<typename R, typename A>
int invoke(A&& a) {
    return impl<R>(Tag{}, forward<A>(a));
}

int main() {
    int x = 9;
    return invoke<int>(x) == 9 ? 0 : 1;
}
