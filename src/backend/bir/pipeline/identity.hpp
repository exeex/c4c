#pragma once

#include "../core/ids.hpp"
#include "../core/result.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace c4c::backend::bir {

struct ModuleRevision {
  std::uint64_t value = 0;
};

struct FunctionRevision {
  std::uint64_t value = 0;
};

constexpr bool operator==(ModuleRevision lhs, ModuleRevision rhs) noexcept {
  return lhs.value == rhs.value;
}
constexpr bool operator!=(ModuleRevision lhs, ModuleRevision rhs) noexcept {
  return !(lhs == rhs);
}
constexpr bool operator==(FunctionRevision lhs, FunctionRevision rhs) noexcept {
  return lhs.value == rhs.value;
}
constexpr bool operator!=(FunctionRevision lhs, FunctionRevision rhs) noexcept {
  return !(lhs == rhs);
}

struct Fingerprint128 {
  std::uint64_t high = 0;
  std::uint64_t low = 0;
};

constexpr bool operator==(Fingerprint128 lhs, Fingerprint128 rhs) noexcept {
  return lhs.high == rhs.high && lhs.low == rhs.low;
}
constexpr bool operator!=(Fingerprint128 lhs, Fingerprint128 rhs) noexcept {
  return !(lhs == rhs);
}

struct FunctionRevisionDigest {
  Fingerprint128 fingerprint{};
};

constexpr bool operator==(FunctionRevisionDigest lhs,
                          FunctionRevisionDigest rhs) noexcept {
  return lhs.fingerprint == rhs.fingerprint;
}
constexpr bool operator!=(FunctionRevisionDigest lhs,
                          FunctionRevisionDigest rhs) noexcept {
  return !(lhs == rhs);
}

struct FunctionRevisionEntry {
  FunctionId function{};
  FunctionRevision revision{};
};

enum class FunctionRevisionDigestErrorCode : std::uint8_t {
  InvalidModuleEpoch,
  InvalidFunctionId,
  ForeignFunctionId,
  DuplicateFunctionSlot,
};

struct FunctionRevisionDigestError {
  FunctionRevisionDigestErrorCode code =
      FunctionRevisionDigestErrorCode::InvalidModuleEpoch;
  std::size_t index = 0;
  FunctionId function{};
};

// The caller supplies core's canonical module function order. This
// identity-only helper preserves that sequence and never sorts or normalizes it.
Result<FunctionRevisionDigest, FunctionRevisionDigestError>
compute_function_revision_digest(
    ModuleEpoch module_epoch,
    const std::vector<FunctionRevisionEntry>& canonical_functions);

struct PipelineStageStamp {
  ModuleEpoch epoch = 0;
  ModuleRevision module_revision{};
  FunctionRevisionDigest function_revisions{};
};

constexpr bool operator==(PipelineStageStamp lhs,
                          PipelineStageStamp rhs) noexcept {
  return lhs.epoch == rhs.epoch &&
         lhs.module_revision == rhs.module_revision &&
         lhs.function_revisions == rhs.function_revisions;
}
constexpr bool operator!=(PipelineStageStamp lhs,
                          PipelineStageStamp rhs) noexcept {
  return !(lhs == rhs);
}

static_assert(std::is_trivially_copyable_v<ModuleRevision>);
static_assert(std::is_trivially_copyable_v<FunctionRevision>);
static_assert(std::is_trivially_copyable_v<Fingerprint128>);
static_assert(std::is_trivially_copyable_v<FunctionRevisionDigest>);
static_assert(std::is_trivially_copyable_v<PipelineStageStamp>);

}  // namespace c4c::backend::bir
