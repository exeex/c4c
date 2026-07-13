#include "execution_control.hpp"

#include <atomic>
#include <utility>

namespace c4c::backend::bir {
namespace detail {

struct ExecutionControlState {
  explicit ExecutionControlState(ResourceBudget input_budget)
      : budget(input_budget), exhausted(input_budget.max_work_units == 0) {}

  const ResourceBudget budget;
  std::atomic<bool> cancellation_requested{false};
  std::atomic<bool> exhausted{false};
  std::atomic<std::uint64_t> work_units_consumed{0};
};

}  // namespace detail
namespace {

Result<void, ExecutionControlFailure> failure(
    ExecutionControlFailure reason) noexcept {
  return Result<void, ExecutionControlFailure>::failure(reason);
}

Result<void, ExecutionControlFailure> success() noexcept {
  return Result<void, ExecutionControlFailure>::success();
}

}  // namespace

CancellationToken::CancellationToken(
    std::shared_ptr<detail::ExecutionControlState> state) noexcept
    : state_(std::move(state)) {}

bool CancellationToken::is_cancelled() const noexcept {
  return state_->cancellation_requested.load(std::memory_order_acquire);
}

std::uint64_t CancellationToken::work_units_consumed() const noexcept {
  return state_->work_units_consumed.load(std::memory_order_relaxed);
}

Result<void, ExecutionControlFailure> CancellationToken::checkpoint(
    std::uint64_t work_units) const noexcept {
  if (is_cancelled()) return failure(ExecutionControlFailure::Cancelled);
  if (state_->exhausted.load(std::memory_order_acquire)) {
    if (is_cancelled()) return failure(ExecutionControlFailure::Cancelled);
    return failure(ExecutionControlFailure::ResourceLimit);
  }
  if (work_units == 0) return success();

  auto consumed =
      state_->work_units_consumed.load(std::memory_order_relaxed);
  while (true) {
    if (work_units > state_->budget.max_work_units - consumed) {
      state_->exhausted.store(true, std::memory_order_release);
      if (is_cancelled()) return failure(ExecutionControlFailure::Cancelled);
      return failure(ExecutionControlFailure::ResourceLimit);
    }

    const auto updated = consumed + work_units;
    if (state_->work_units_consumed.compare_exchange_weak(
            consumed, updated, std::memory_order_acq_rel,
            std::memory_order_relaxed)) {
      if (updated == state_->budget.max_work_units)
        state_->exhausted.store(true, std::memory_order_release);
      if (is_cancelled()) return failure(ExecutionControlFailure::Cancelled);
      return success();
    }

    if (is_cancelled()) return failure(ExecutionControlFailure::Cancelled);
    if (state_->exhausted.load(std::memory_order_acquire)) {
      if (is_cancelled()) return failure(ExecutionControlFailure::Cancelled);
      return failure(ExecutionControlFailure::ResourceLimit);
    }
  }
}

CancellationSource::CancellationSource(ResourceBudget budget)
    : state_(std::make_shared<detail::ExecutionControlState>(budget)) {}

CancellationToken CancellationSource::token() const noexcept {
  return CancellationToken(state_);
}

void CancellationSource::request_cancellation() noexcept {
  state_->cancellation_requested.store(true, std::memory_order_release);
}

}  // namespace c4c::backend::bir
