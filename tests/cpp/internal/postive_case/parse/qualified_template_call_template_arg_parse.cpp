// Parse-only regression: call-like qualified template expressions inside
// template argument lists should go straight to non-type parsing instead of
// paying the speculative type-id probe first.
// RUN: %c4cll --parse-only %s

template <bool B, typename T = void>
struct enable_if {
  using type = T;
};

template <typename A, typename B>
struct is_same {
  static constexpr bool value = false;
};

template <typename T1, typename T2>
struct pair_like {
  template <bool, typename U1, typename U2>
  struct chooser {
    template <typename X1, typename X2>
    static constexpr bool check() {
      return true;
    }
  };

  template <typename U1, typename U2>
  using chooser_t =
      chooser<!is_same<T1, U1>::value || !is_same<T2, U2>::value, U1, U2>;

  template <typename U1, typename U2,
            typename enable_if<
                chooser_t<U1, U2>::template check<U1, U2>(), bool>::type = true>
  static void use();
};

int main() {
  return 0;
}
