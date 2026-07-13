#pragma once

#include "../core/ir.hpp"

#include <string>
#include <variant>
#include <vector>

namespace c4c::backend::bir {

enum class VerifyProfile { FoundationRaw };

enum class VerificationRule {
  ModuleEpoch,
  FunctionStorageAndOrder,
  BlockStorageAndOrder,
  InstructionStorageAndOrder,
  ParameterDefinition,
  ValueDefinition,
  FunctionShape,
  Terminator,
  LinkNameIndex,
  BoundedAlternative,
};

struct ModuleEntity {};
struct LinkNameEntity {
  std::string name;
};
using VerificationEntity =
    std::variant<ModuleEntity, FunctionId, BlockId, InstId, ValueId,
                 LinkNameEntity>;

struct VerificationError {
  VerificationRule rule = VerificationRule::ModuleEpoch;
  FunctionId function{};
  VerificationEntity entity = ModuleEntity{};
  std::string message;
};

struct VerificationResult {
  std::vector<VerificationError> errors;

  bool ok() const noexcept { return errors.empty(); }
  explicit operator bool() const noexcept { return ok(); }
};

class FoundationVerifier {
 public:
  static VerificationResult verify(const detail::ModuleData& module,
                                   VerifyProfile profile =
                                       VerifyProfile::FoundationRaw);
};

}  // namespace c4c::backend::bir
