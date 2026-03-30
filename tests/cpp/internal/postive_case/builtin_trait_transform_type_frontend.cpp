// Frontend regression: builtin transform traits should be usable as type
// arguments to builtin predicate traits.

static_assert(__is_same(__remove_cv(const int), int));
static_assert(__is_same(__remove_cvref(const int&), int));
static_assert(__is_same(__remove_reference_t(int&&), int));

int main() {
  return __is_same(__remove_cv(const int), int) &&
                 __is_same(__remove_cvref(const int&), int) &&
                 __is_same(__remove_reference_t(int&&), int)
             ? 0
             : 1;
}
