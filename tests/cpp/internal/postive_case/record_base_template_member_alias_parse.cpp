// Parse-only regression: a `using` alias should accept `::template` member
// lookups on a record-qualified owner before the final `::type`.
// RUN: %c4cll --parse-only %s

struct allocator_traits_base {
  template<typename Alloc, typename Up>
  struct rebind {
    using type = Up;
  };
};

template<typename Alloc, typename Up>
using alloc_rebind =
    typename allocator_traits_base::template rebind<Alloc, Up>::type;

alloc_rebind<int, long> make_rebound();

int main() {
  return 0;
}
