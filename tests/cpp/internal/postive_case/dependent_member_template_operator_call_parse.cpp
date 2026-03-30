// Parse-only regression: dependent member-template operator() calls should be
// accepted after `.` in expression context.
// RUN: %c4cll --parse-only %s

template <unsigned long... Index>
struct index_sequence {};

struct probe_fn {
  template <unsigned long I>
  void operator()() const {}
};

template <unsigned long... Index, class Function>
void for_each_index_sequence(index_sequence<Index...>, Function func) {
  (func.template operator()<Index>(), ...);
}

int main() {
  for_each_index_sequence(index_sequence<0, 1, 2>{}, probe_fn{});
  return 0;
}
