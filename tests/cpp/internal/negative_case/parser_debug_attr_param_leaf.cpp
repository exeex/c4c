// Parser-debug regression: keep the committed top-level parameter-list
// summary when an attributed parameter starts with `[` while still preserving
// the qualified-type probe suffix in debug mode.

template <class T>
struct Alloc {};

template <class T>
void relocate(T* first, T* last, T* result,
              [[maybe_unused]] Alloc<T>& alloc) noexcept;
