#include "identity.hpp"

namespace c4c::backend::bir {
namespace {

class StableDigest128 {
 public:
  void add(std::uint64_t value) noexcept {
    for (unsigned shift = 0; shift != 64; shift += 8) {
      const auto byte = static_cast<std::uint8_t>(value >> shift);
      low_ ^= byte;
      low_ *= UINT64_C(1099511628211);

      high_ ^= byte;
      high_ *= UINT64_C(14029467366897019727);
      high_ ^= high_ >> 32U;
    }
  }

  Fingerprint128 finish() const noexcept { return Fingerprint128{high_, low_}; }

 private:
  std::uint64_t high_ = UINT64_C(7809847782465536322);
  std::uint64_t low_ = UINT64_C(14695981039346656037);
};

FunctionRevisionDigestError error(FunctionRevisionDigestErrorCode code,
                                  std::size_t index,
                                  FunctionId function = {}) {
  return FunctionRevisionDigestError{code, index, function};
}

}  // namespace

Result<FunctionRevisionDigest, FunctionRevisionDigestError>
compute_function_revision_digest(
    ModuleEpoch module_epoch,
    const std::vector<FunctionRevisionEntry>& canonical_functions) {
  if (module_epoch == 0) {
    return Result<FunctionRevisionDigest,
                  FunctionRevisionDigestError>::failure(
        error(FunctionRevisionDigestErrorCode::InvalidModuleEpoch, 0));
  }

  StableDigest128 digest;
  digest.add(UINT64_C(0x4334434652455631));  // "C4CFREV1"
  digest.add(module_epoch);
  digest.add(static_cast<std::uint64_t>(canonical_functions.size()));

  for (std::size_t index = 0; index < canonical_functions.size(); ++index) {
    const auto& entry = canonical_functions[index];
    if (!entry.function.valid()) {
      return Result<FunctionRevisionDigest,
                    FunctionRevisionDigestError>::failure(
          error(FunctionRevisionDigestErrorCode::InvalidFunctionId, index,
                entry.function));
    }
    if (entry.function.epoch != module_epoch) {
      return Result<FunctionRevisionDigest,
                    FunctionRevisionDigestError>::failure(
          error(FunctionRevisionDigestErrorCode::ForeignFunctionId, index,
                entry.function));
    }
    for (std::size_t prior = 0; prior < index; ++prior) {
      if (canonical_functions[prior].function.slot == entry.function.slot) {
        return Result<FunctionRevisionDigest,
                      FunctionRevisionDigestError>::failure(
            error(FunctionRevisionDigestErrorCode::DuplicateFunctionSlot,
                  index, entry.function));
      }
    }

    digest.add(entry.function.epoch);
    digest.add(entry.function.slot);
    digest.add(entry.function.generation);
    digest.add(entry.revision.value);
  }

  return Result<FunctionRevisionDigest,
                FunctionRevisionDigestError>::success(
      FunctionRevisionDigest{digest.finish()});
}

}  // namespace c4c::backend::bir
