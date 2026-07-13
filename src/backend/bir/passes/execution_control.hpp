#pragma once

#include "../core/result.hpp"

#include <cstdint>
#include <memory>

namespace c4c::backend::bir {

struct ResourceBudget {
  // This foundation intentionally implements only the deterministic work axis.
  std::uint64_t max_work_units = 0;
};

enum class ExecutionControlFailure : std::uint8_t {
  Cancelled,
  ResourceLimit,
};

namespace detail {
struct ExecutionControlState;
}  // namespace detail

class CancellationSource;

class CancellationToken {
 public:
  bool is_cancelled() const noexcept;
  std::uint64_t work_units_consumed() const noexcept;

  // Cancellation has precedence over resource exhaustion. A zero-unit
  // checkpoint observes sticky state without charging work. Checkpoint calls
  // for one invocation are serial; cancellation requests may be concurrent.
  Result<void, ExecutionControlFailure> checkpoint(
      std::uint64_t work_units = 1) const noexcept;

 private:
  explicit CancellationToken(
      std::shared_ptr<detail::ExecutionControlState> state) noexcept;

  std::shared_ptr<detail::ExecutionControlState> state_;

  friend class CancellationSource;
};

class CancellationSource {
 public:
  explicit CancellationSource(ResourceBudget budget);

  CancellationSource(const CancellationSource&) = delete;
  CancellationSource& operator=(const CancellationSource&) = delete;
  CancellationSource(CancellationSource&&) = delete;
  CancellationSource& operator=(CancellationSource&&) = delete;

  CancellationToken token() const noexcept;
  void request_cancellation() noexcept;

 private:
  std::shared_ptr<detail::ExecutionControlState> state_;
};

}  // namespace c4c::backend::bir
