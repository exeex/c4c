namespace detail {
template<typename T>
struct holder {
  using type = T;
};
}

template<typename T>
struct detail::holder<T*> {
  using type = T;
};

