#pragma once

#include "bir/lir_to_bir.hpp"
#include "target_profile.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend {

class BackendModuleInput {
 public:
  explicit BackendModuleInput(
      const c4c::codegen::lir::LirModule& lir_module) noexcept
      : lir_module_(lir_module) {}

  const c4c::codegen::lir::LirModule& lir_module() const noexcept {
    return lir_module_.get();
  }

 private:
  std::reference_wrapper<const c4c::codegen::lir::LirModule> lir_module_;
};

struct BackendOptions {
  c4c::TargetProfile target_profile{};
  bool emit_semantic_bir = false;
  std::optional<std::string> route_debug_focus_function;
  std::optional<std::string> route_debug_focus_block;
  std::optional<std::string> route_debug_focus_value;
};

// Later stages remain named so active command-line callers compile while the
// new BIR-to-MIR boundary is being designed. They fail explicitly at runtime.
enum class BackendDumpStage {
  SemanticBir,
  PreparedBir,
  MirSummary,
  MirTrace,
};

struct BackendObjectResult {
  std::vector<std::uint8_t> bytes;
  std::string diagnostic;

  [[nodiscard]] bool ok() const {
    return diagnostic.empty() && !bytes.empty();
  }
};

BackendObjectResult emit_module_object(const BackendModuleInput& input,
                                       const BackendOptions& options);

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options);

std::string dump_module(const BackendModuleInput& input,
                        const BackendOptions& options,
                        BackendDumpStage stage);

}  // namespace c4c::backend
