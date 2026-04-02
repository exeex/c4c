// Parser-debug regression: keep manual template-base token injection visible
// when a later top-level parse failure is the committed diagnostic.

template <class T>
struct Base {};

template <class T>
struct Derived : Base<T> {};

Derived<int> value(
