#pragma once

#include "../core/builder.hpp"
#include "../passes/execution_control.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace c4c::backend::bir {

enum class CheckpointAcquireFailure : std::uint8_t {
  ConsumedRaw,
};

enum class PipelineCapabilityFailure : std::uint8_t {
  Consumed,
};

enum class OccurrenceForkFailure : std::uint8_t {
  ConsumedCheckpoint,
  Cancelled,
  ResourceLimit,
  AllocationFailure,
};

class PrivateOccurrenceCandidate;

namespace pipeline_internal {
struct CheckpointAccess;
struct ForkAccess;
}  // namespace pipeline_internal

// Internal pipeline capability: owns one verified last-good Raw occurrence.
// This header is intentionally not exported through bir.hpp.
class PipelineCheckpoint {
 public:
  PipelineCheckpoint(PipelineCheckpoint&&) noexcept = default;
  PipelineCheckpoint& operator=(PipelineCheckpoint&&) noexcept = default;
  PipelineCheckpoint(const PipelineCheckpoint&) = delete;
  PipelineCheckpoint& operator=(const PipelineCheckpoint&) = delete;

  Result<ModuleView, PipelineCapabilityFailure> view() const;
  Result<PipelineStageStamp, PipelineCapabilityFailure> stage_stamp() const;

 private:
  explicit PipelineCheckpoint(std::unique_ptr<detail::ModuleData> data)
      : data_(std::move(data)) {}

  std::unique_ptr<detail::ModuleData> data_;

  friend struct pipeline_internal::CheckpointAccess;
  friend struct pipeline_internal::ForkAccess;
};

// Internal, private owning occurrence. This packet intentionally supports only
// immutable inspection and discard; it has no edit, promotion, or publication
// authority.
class PrivateOccurrenceCandidate {
 public:
  PrivateOccurrenceCandidate(PrivateOccurrenceCandidate&&) noexcept = default;
  PrivateOccurrenceCandidate& operator=(PrivateOccurrenceCandidate&&) noexcept =
      default;
  PrivateOccurrenceCandidate(const PrivateOccurrenceCandidate&) = delete;
  PrivateOccurrenceCandidate& operator=(const PrivateOccurrenceCandidate&) =
      delete;

  Result<ModuleView, PipelineCapabilityFailure> view() const;
  Result<PipelineStageStamp, PipelineCapabilityFailure> stage_stamp() const;
  Result<void, PipelineCapabilityFailure> discard() noexcept;

 private:
  explicit PrivateOccurrenceCandidate(
      std::unique_ptr<detail::ModuleData> data)
      : data_(std::move(data)) {}

  std::unique_ptr<detail::ModuleData> data_;

  friend struct pipeline_internal::ForkAccess;
};

namespace pipeline_internal {

Result<PipelineCheckpoint, CheckpointAcquireFailure> consume_verified_raw(
    RawBir&& raw);

// Deterministic cost: one preflight unit, one unit per function in canonical
// module order, then one post-clone unit before the candidate may escape.
Result<PrivateOccurrenceCandidate, OccurrenceForkFailure> fork_occurrence(
    const PipelineCheckpoint& checkpoint, const CancellationToken& control);

// Internal proof seam: observes only whether the two owning allocations differ.
bool has_distinct_storage(const PipelineCheckpoint& checkpoint,
                          const PrivateOccurrenceCandidate& candidate) noexcept;

}  // namespace pipeline_internal

}  // namespace c4c::backend::bir
