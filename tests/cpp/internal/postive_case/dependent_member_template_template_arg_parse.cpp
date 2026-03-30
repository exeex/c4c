// Parse-only regression: dependent member templates should be accepted as
// template-template arguments in type context.
// RUN: %c4cll --parse-only %s

template<typename T>
struct xref {
  template<typename U>
  using apply [[__gnu__::__nodebug__]] = U;
};

template<typename T, typename U, template<typename> class XT,
         template<typename> class YT>
struct basic_common_reference {
  using type = int;
};

template<typename T, typename U>
using basic_common_reference_t [[__gnu__::__nodebug__]] =
    typename basic_common_reference<T, U, xref<T>::template apply,
                                    xref<U>::template apply>::type;

template<typename T, typename U>
  requires requires { typename basic_common_reference_t<T, U>; }
struct probe {
  using type = basic_common_reference_t<T, U>;
};

int main() {
  return 0;
}
