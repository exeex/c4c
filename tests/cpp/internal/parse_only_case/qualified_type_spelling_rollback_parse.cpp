// RUN: %c4cll --parse-only %s

template <typename T>
void probe() {
    T::value(1);
    typename T::type kept = 0;
}

struct Example {
    using type = int;
    static void value(int);
};

void instantiate() {
    probe<Example>();
}
