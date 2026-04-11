// Candidate:
// - a compile-time lowering rule table is expressed as template
//   specializations
// - a consteval query reaches that rule before the matching specialization is
//   visible
//
// c4c angle:
// - this looks like a tiny compile-time dispatcher for op legalization

template <int Op>
struct rule;

template <int Op>
consteval int legal_vector_width() {
  return rule<Op>::width;
}

template <int Op, int W = legal_vector_width<Op>()>
struct lowered_kernel {
  static constexpr int width = W;
};

constexpr int width = lowered_kernel<3>::width;

template <>
struct rule<3> {
  static constexpr int width = 16;
};

static_assert(width == 16);

int main() {
  return 0;
}
