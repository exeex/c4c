#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/pipeline/checkpoint_internal.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace bir = c4c::backend::bir;
namespace internal = c4c::backend::bir::pipeline_internal;

namespace {

[[noreturn]] void fail(const std::string& message) {
  std::cerr << "FAIL: " << message << '\n';
  std::exit(1);
}

void expect(bool condition, const std::string& message) {
  if (!condition) fail(message);
}

bir::RawBir make_raw() {
  bir::ModuleBuilder builder;
  const bir::FunctionSignature signature{
      bir::Type{bir::TypeKind::Void}, {}, false};
  expect(builder.create_function(signature, "declaration", true).has_value(),
         "declaration construction must succeed");
  auto definition =
      builder.create_function(signature, "definition", false);
  expect(definition.has_value(), "definition construction must succeed");
  auto edited = builder.with_function(
      definition.value(), [](bir::FunctionBuilder& function) {
        auto block = function.create_block("entry");
        if (!block)
          return bir::Result<void, bir::BuildError>::failure(block.error());
        return function.set_terminator(block.value(), bir::ReturnTerm{});
      });
  expect(edited.has_value(), "definition body must be valid");
  auto published = std::move(builder).publish();
  expect(published.has_value(), "valid module must publish");
  return std::move(published).value();
}

bir::PipelineCheckpoint make_checkpoint() {
  auto raw = make_raw();
  auto checkpoint = internal::consume_verified_raw(std::move(raw));
  expect(checkpoint.has_value(), "verified RawBir must be consumable once");
  auto consumed_again = internal::consume_verified_raw(std::move(raw));
  expect(!consumed_again.has_value() &&
             consumed_again.error() == bir::CheckpointAcquireFailure::ConsumedRaw,
         "consumed RawBir must return a typed failure on reuse");
  return std::move(checkpoint).value();
}

void expect_checkpoint_unchanged(
    const bir::PipelineCheckpoint& checkpoint,
    bir::PipelineStageStamp stamp,
    const std::vector<bir::FunctionId>& functions,
    const std::string& context) {
  auto current_stamp = checkpoint.stage_stamp();
  auto current_view = checkpoint.view();
  expect(current_stamp.has_value() && current_stamp.value() == stamp,
         context + " must preserve the checkpoint stamp");
  expect(current_view.has_value() && current_view.value().functions() == functions,
         context + " must preserve checkpoint storage and order");
}

void test_move_only_consume_once_and_exact_identity() {
  static_assert(!std::is_copy_constructible_v<bir::PipelineCheckpoint>);
  static_assert(std::is_move_constructible_v<bir::PipelineCheckpoint>);
  static_assert(!std::is_copy_constructible_v<bir::PrivateOccurrenceCandidate>);
  static_assert(std::is_move_constructible_v<bir::PrivateOccurrenceCandidate>);
  static_assert(!std::is_constructible_v<bir::CanonicalBir,
                                         bir::PrivateOccurrenceCandidate&&>);
  static_assert(!std::is_constructible_v<bir::PipelineCheckpoint,
                                         std::unique_ptr<bir::detail::ModuleData>>);
  static_assert(!std::is_constructible_v<bir::PrivateOccurrenceCandidate,
                                         std::unique_ptr<bir::detail::ModuleData>>);

  auto checkpoint = make_checkpoint();
  auto original_view = checkpoint.view();
  auto original_stamp = checkpoint.stage_stamp();
  expect(original_view.has_value() && original_stamp.has_value(),
         "live checkpoint must expose immutable view and exact stamp");
  expect(original_view.value().functions().size() == 2,
         "checkpoint must own the declaration and definition");

  auto moved = std::move(checkpoint);
  expect(!checkpoint.view().has_value() &&
             checkpoint.view().error() == bir::PipelineCapabilityFailure::Consumed,
         "moved-from checkpoint view must fail safely");
  bir::CancellationSource control(bir::ResourceBudget{4});
  auto moved_fork = internal::fork_occurrence(checkpoint, control.token());
  expect(!moved_fork.has_value() &&
             moved_fork.error() == bir::OccurrenceForkFailure::ConsumedCheckpoint,
         "moved-from checkpoint fork must return a typed failure");
  expect(moved.stage_stamp().has_value() &&
             moved.stage_stamp().value() == original_stamp.value(),
         "moving checkpoint must preserve exact owned identity");
}

void test_deep_repeated_forks_and_discard() {
  auto checkpoint = make_checkpoint();
  const auto stamp = checkpoint.stage_stamp().value();
  const auto functions = checkpoint.view().value().functions();

  bir::CancellationSource first_control(bir::ResourceBudget{4});
  auto first_result =
      internal::fork_occurrence(checkpoint, first_control.token());
  expect(first_result.has_value(), "exact fork budget must succeed");
  auto first = std::move(first_result).value();
  expect(first_control.token().work_units_consumed() == 4,
         "two-function fork must charge exactly 2 + function_count units");
  expect(internal::has_distinct_storage(checkpoint, first),
         "candidate must deeply own storage distinct from checkpoint");
  expect(first.stage_stamp().has_value() && first.stage_stamp().value() == stamp,
         "candidate must retain the exact pre-mutation stamp");
  expect(first.view().has_value() && first.view().value().functions() == functions,
         "candidate view must reproduce the checkpoint occurrence");
  expect_checkpoint_unchanged(checkpoint, stamp, functions,
                              "successful fork");

  expect(first.discard().has_value(), "explicit discard must succeed once");
  expect(!first.discard().has_value() &&
             first.discard().error() == bir::PipelineCapabilityFailure::Consumed,
         "discarded candidate reuse must return typed consumed failure");
  expect_checkpoint_unchanged(checkpoint, stamp, functions,
                              "explicit candidate discard");

  bir::CancellationSource second_control(bir::ResourceBudget{4});
  auto second_result =
      internal::fork_occurrence(checkpoint, second_control.token());
  expect(second_result.has_value(),
         "same checkpoint must support repeated identical private forks");
  auto second = std::move(second_result).value();
  expect(second.stage_stamp().value() == stamp &&
             second.view().value().functions() == functions,
         "repeated successful fork must reproduce view and stamp");
  auto moved_second = std::move(second);
  expect(!second.view().has_value() &&
             second.view().error() == bir::PipelineCapabilityFailure::Consumed,
         "moved-from candidate view must return typed consumed failure");
  expect(moved_second.stage_stamp().has_value() &&
             moved_second.stage_stamp().value() == stamp,
         "moving a candidate must preserve its exact private occurrence");
}

void test_failures_leave_checkpoint_reusable() {
  auto checkpoint = make_checkpoint();
  const auto stamp = checkpoint.stage_stamp().value();
  const auto functions = checkpoint.view().value().functions();

  bir::CancellationSource cancelled(bir::ResourceBudget{4});
  cancelled.request_cancellation();
  auto cancelled_fork =
      internal::fork_occurrence(checkpoint, cancelled.token());
  expect(!cancelled_fork.has_value() &&
             cancelled_fork.error() == bir::OccurrenceForkFailure::Cancelled,
         "cancellation before fork must fail with typed cancellation");
  expect(cancelled.token().work_units_consumed() == 0,
         "preflight cancellation must not charge work");
  expect_checkpoint_unchanged(checkpoint, stamp, functions,
                              "preflight cancellation");

  bir::CancellationSource before_clone_limit(bir::ResourceBudget{1});
  auto before_clone =
      internal::fork_occurrence(checkpoint, before_clone_limit.token());
  expect(!before_clone.has_value() &&
             before_clone.error() == bir::OccurrenceForkFailure::ResourceLimit,
         "budget below sequence charge must fail before cloning");
  expect_checkpoint_unchanged(checkpoint, stamp, functions,
                              "pre-clone resource failure");

  bir::CancellationSource after_clone_limit(bir::ResourceBudget{3});
  auto after_clone =
      internal::fork_occurrence(checkpoint, after_clone_limit.token());
  expect(!after_clone.has_value() &&
             after_clone.error() == bir::OccurrenceForkFailure::ResourceLimit,
         "budget missing post-clone unit must discard the private clone");
  expect(after_clone_limit.token().work_units_consumed() == 3,
         "post-clone limit must charge only the deterministic completed work");
  expect_checkpoint_unchanged(checkpoint, stamp, functions,
                              "post-clone resource failure");

  bir::CancellationSource retry_control(bir::ResourceBudget{4});
  auto retry = internal::fork_occurrence(checkpoint, retry_control.token());
  expect(retry.has_value(), "checkpoint must remain reusable after every failure");
  expect(retry.value().stage_stamp().value() == stamp &&
             retry.value().view().value().functions() == functions,
         "retry after failures must reproduce exact view and stamp");
}

}  // namespace

int main() {
  test_move_only_consume_once_and_exact_identity();
  test_deep_repeated_forks_and_discard();
  test_failures_leave_checkpoint_reusable();
  return 0;
}
