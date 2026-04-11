// Candidate:
// - schedule<Op>() depends on cost<Op>()
// - the relevant explicit specialization appears later in the translation unit
//
// Mainstream C++:
// - rejects because cost<Op>() is used before the specialization body exists
//
// c4c angle:
// - treat the missing specialization result as unresolved
// - converge once the explicit specialization is seen

template <int Op>
consteval int cost();

template <int Op>
consteval int schedule() {
  return cost<Op>() + 1;
}

template <int Op, int V = schedule<Op>()>
struct plan {
  static constexpr int value = V;
};

constexpr int schedule_cost = plan<7>::value;

template <>
consteval int cost<7>() {
  return 41;
}

static_assert(schedule_cost == 42);

int main() {
  return 0;
}
