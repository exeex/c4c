#include "src/backend/bir/bir.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

namespace bir = c4c::backend::bir;

namespace {

[[noreturn]] void fail(const std::string& message) {
  std::cerr << "FAIL: " << message << '\n';
  std::exit(1);
}

void expect(bool condition, const std::string& message) {
  if (!condition) fail(message);
}

void expect_success(
    const bir::Result<void, bir::ExecutionControlFailure>& result,
    const std::string& message) {
  expect(result.has_value(), message);
}

void expect_failure(
    const bir::Result<void, bir::ExecutionControlFailure>& result,
    bir::ExecutionControlFailure expected, const std::string& message) {
  expect(!result.has_value() && result.error() == expected, message);
}

bir::CancellationToken surviving_token(std::uint64_t max_work_units) {
  bir::CancellationSource source{bir::ResourceBudget{max_work_units}};
  return source.token();
}

void test_independent_sources_and_token_lifetime() {
  static_assert(!std::is_default_constructible_v<bir::CancellationToken>);
  static_assert(!std::is_copy_constructible_v<bir::CancellationSource>);
  static_assert(!std::is_move_constructible_v<bir::CancellationSource>);

  bir::CancellationSource first{bir::ResourceBudget{4}};
  bir::CancellationSource second{bir::ResourceBudget{4}};
  const auto first_token = first.token();
  const auto second_token = second.token();

  first.request_cancellation();
  first.request_cancellation();
  expect(first_token.is_cancelled(),
         "cancellation request must be visible and idempotent");
  expect(!second_token.is_cancelled(),
         "independent cancellation sources must not share state");
  expect_failure(first_token.checkpoint(),
                 bir::ExecutionControlFailure::Cancelled,
                 "cancelled token must fail without charging work");
  expect_failure(first_token.checkpoint(0),
                 bir::ExecutionControlFailure::Cancelled,
                 "cancelled token must never later report success");
  expect(first_token.work_units_consumed() == 0,
         "cancelled checkpoints must not charge work");
  expect_success(second_token.checkpoint(),
                 "independent token must remain usable");

  const auto survivor = surviving_token(2);
  expect_success(survivor.checkpoint(2),
                 "token must safely outlive its owning source");
  expect(survivor.work_units_consumed() == 2,
         "surviving token must retain shared accounting state");
}

void test_exact_boundary_zero_units_and_sticky_exhaustion() {
  bir::CancellationSource source{bir::ResourceBudget{5}};
  const auto token = source.token();

  expect_success(token.checkpoint(0),
                 "zero-unit checkpoint must observe without charging");
  expect(token.work_units_consumed() == 0,
         "zero-unit checkpoint must not consume work");
  expect_success(token.checkpoint(2), "partial boundary charge must succeed");
  expect_success(token.checkpoint(3), "exact boundary charge must succeed");
  expect(token.work_units_consumed() == 5,
         "exact boundary must account every charged unit");
  expect_failure(token.checkpoint(0),
                 bir::ExecutionControlFailure::ResourceLimit,
                 "zero-unit observation must report sticky exhaustion");
  expect_failure(token.checkpoint(1),
                 bir::ExecutionControlFailure::ResourceLimit,
                 "exhausted token must never later report success");

  bir::CancellationSource empty{bir::ResourceBudget{0}};
  expect_failure(empty.token().checkpoint(0),
                 bir::ExecutionControlFailure::ResourceLimit,
                 "zero budget must begin exhausted even for observation");
}

void test_over_budget_and_overflow_rejection() {
  bir::CancellationSource over_budget{bir::ResourceBudget{4}};
  const auto over_budget_token = over_budget.token();
  expect_failure(over_budget_token.checkpoint(5),
                 bir::ExecutionControlFailure::ResourceLimit,
                 "over-budget charge must fail atomically");
  expect(over_budget_token.work_units_consumed() == 0,
         "rejected charge must not publish partial accounting");
  expect_failure(over_budget_token.checkpoint(0),
                 bir::ExecutionControlFailure::ResourceLimit,
                 "over-budget failure must remain sticky");

  constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
  bir::CancellationSource overflow{bir::ResourceBudget{maximum}};
  const auto overflow_token = overflow.token();
  expect_success(overflow_token.checkpoint(maximum - 1),
                 "largest in-budget partial charge must succeed");
  expect_failure(overflow_token.checkpoint(2),
                 bir::ExecutionControlFailure::ResourceLimit,
                 "addition beyond uint64 range must fail without wraparound");
  expect(overflow_token.work_units_consumed() == maximum - 1,
         "overflow rejection must preserve prior exact accounting");
}

void test_cancellation_precedence_and_repeated_determinism() {
  bir::CancellationSource exhausted{bir::ResourceBudget{1}};
  const auto exhausted_token = exhausted.token();
  expect_success(exhausted_token.checkpoint(1),
                 "boundary charge should succeed before exhaustion is observed");
  exhausted.request_cancellation();
  expect_failure(exhausted_token.checkpoint(0),
                 bir::ExecutionControlFailure::Cancelled,
                 "observed cancellation must take precedence over exhaustion");

  std::vector<std::uint64_t> observed;
  for (int run = 0; run != 4; ++run) {
    bir::CancellationSource source{bir::ResourceBudget{6}};
    const auto token = source.token();
    expect_success(token.checkpoint(1), "deterministic charge 1 must succeed");
    expect_success(token.checkpoint(2), "deterministic charge 2 must succeed");
    expect_success(token.checkpoint(3), "deterministic charge 3 must succeed");
    expect_failure(token.checkpoint(),
                   bir::ExecutionControlFailure::ResourceLimit,
                   "same deterministic sequence must exhaust identically");
    observed.push_back(token.work_units_consumed());
  }
  expect(observed == std::vector<std::uint64_t>({6, 6, 6, 6}),
         "repeated executions must produce identical accounting");
}

}  // namespace

int main() {
  test_independent_sources_and_token_lifetime();
  test_exact_boundary_zero_units_and_sticky_exhaustion();
  test_over_budget_and_overflow_rejection();
  test_cancellation_precedence_and_repeated_determinism();
  return 0;
}
