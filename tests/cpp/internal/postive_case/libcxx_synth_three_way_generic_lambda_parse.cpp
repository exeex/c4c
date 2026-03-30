// Parse-only regression: libc++ synth_three_way uses a generic lambda with a
// template parameter list and trailing requires-clause.
// RUN: %c4cll --parse-only %s

inline constexpr auto __synth_three_way = []<class _Tp, class _Up>(
                                               const _Tp& __t,
                                               const _Up& __u)
  requires requires {
    __t < __u;
    __u < __t;
  }
{
  return 0;
};

int main() {
  return __synth_three_way(1, 2);
}
