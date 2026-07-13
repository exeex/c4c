#include "checkpoint_internal.hpp"

#include <cstddef>
#include <limits>
#include <new>
#include <utility>

namespace c4c::backend::bir {
namespace {

OccurrenceForkFailure fork_failure(ExecutionControlFailure failure) noexcept {
  return failure == ExecutionControlFailure::Cancelled
             ? OccurrenceForkFailure::Cancelled
             : OccurrenceForkFailure::ResourceLimit;
}

}  // namespace

Result<ModuleView, PipelineCapabilityFailure> PipelineCheckpoint::view() const {
  if (!data_)
    return Result<ModuleView, PipelineCapabilityFailure>::failure(
        PipelineCapabilityFailure::Consumed);
  return Result<ModuleView, PipelineCapabilityFailure>::success(
      ModuleView(*data_));
}

Result<PipelineStageStamp, PipelineCapabilityFailure>
PipelineCheckpoint::stage_stamp() const {
  if (!data_)
    return Result<PipelineStageStamp, PipelineCapabilityFailure>::failure(
        PipelineCapabilityFailure::Consumed);
  return Result<PipelineStageStamp, PipelineCapabilityFailure>::success(
      data_->stage_stamp_);
}

Result<ModuleView, PipelineCapabilityFailure>
PrivateOccurrenceCandidate::view() const {
  if (!data_)
    return Result<ModuleView, PipelineCapabilityFailure>::failure(
        PipelineCapabilityFailure::Consumed);
  return Result<ModuleView, PipelineCapabilityFailure>::success(
      ModuleView(*data_));
}

Result<PipelineStageStamp, PipelineCapabilityFailure>
PrivateOccurrenceCandidate::stage_stamp() const {
  if (!data_)
    return Result<PipelineStageStamp, PipelineCapabilityFailure>::failure(
        PipelineCapabilityFailure::Consumed);
  return Result<PipelineStageStamp, PipelineCapabilityFailure>::success(
      data_->stage_stamp_);
}

Result<void, PipelineCapabilityFailure>
PrivateOccurrenceCandidate::discard() noexcept {
  if (!data_)
    return Result<void, PipelineCapabilityFailure>::failure(
        PipelineCapabilityFailure::Consumed);
  data_.reset();
  return Result<void, PipelineCapabilityFailure>::success();
}

namespace pipeline_internal {

struct CheckpointAccess {
  static Result<PipelineCheckpoint, CheckpointAcquireFailure> consume(
      RawBir&& raw) {
    if (!raw.data_)
      return Result<PipelineCheckpoint, CheckpointAcquireFailure>::failure(
          CheckpointAcquireFailure::ConsumedRaw);
    return Result<PipelineCheckpoint, CheckpointAcquireFailure>::success(
        PipelineCheckpoint(std::move(raw.data_)));
  }
};

struct ForkAccess {
  static Result<PrivateOccurrenceCandidate, OccurrenceForkFailure> fork(
      const PipelineCheckpoint& checkpoint, const CancellationToken& control) {
    if (!checkpoint.data_)
      return Result<PrivateOccurrenceCandidate, OccurrenceForkFailure>::failure(
          OccurrenceForkFailure::ConsumedCheckpoint);

    auto preflight = control.checkpoint(1);
    if (!preflight)
      return Result<PrivateOccurrenceCandidate, OccurrenceForkFailure>::failure(
          fork_failure(preflight.error()));

    const auto function_count =
        checkpoint.data_->function_order_.ids().size();
    if (function_count > std::numeric_limits<std::uint64_t>::max())
      return Result<PrivateOccurrenceCandidate, OccurrenceForkFailure>::failure(
          OccurrenceForkFailure::ResourceLimit);
    auto sequence_charge =
        control.checkpoint(static_cast<std::uint64_t>(function_count));
    if (!sequence_charge)
      return Result<PrivateOccurrenceCandidate, OccurrenceForkFailure>::failure(
          fork_failure(sequence_charge.error()));

    std::unique_ptr<detail::ModuleData> candidate;
    try {
      candidate =
          std::make_unique<detail::ModuleData>(*checkpoint.data_);
    } catch (const std::bad_alloc&) {
      return Result<PrivateOccurrenceCandidate,
                    OccurrenceForkFailure>::failure(
          OccurrenceForkFailure::AllocationFailure);
    }

    auto post_clone = control.checkpoint(1);
    if (!post_clone)
      return Result<PrivateOccurrenceCandidate, OccurrenceForkFailure>::failure(
          fork_failure(post_clone.error()));
    return Result<PrivateOccurrenceCandidate, OccurrenceForkFailure>::success(
        PrivateOccurrenceCandidate(std::move(candidate)));
  }

  static bool distinct(const PipelineCheckpoint& checkpoint,
                       const PrivateOccurrenceCandidate& candidate) noexcept {
    return checkpoint.data_ && candidate.data_ &&
           checkpoint.data_.get() != candidate.data_.get();
  }
};

Result<PipelineCheckpoint, CheckpointAcquireFailure> consume_verified_raw(
    RawBir&& raw) {
  return CheckpointAccess::consume(std::move(raw));
}

Result<PrivateOccurrenceCandidate, OccurrenceForkFailure> fork_occurrence(
    const PipelineCheckpoint& checkpoint, const CancellationToken& control) {
  return ForkAccess::fork(checkpoint, control);
}

bool has_distinct_storage(const PipelineCheckpoint& checkpoint,
                          const PrivateOccurrenceCandidate& candidate) noexcept {
  return ForkAccess::distinct(checkpoint, candidate);
}

}  // namespace pipeline_internal
}  // namespace c4c::backend::bir
