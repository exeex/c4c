#pragma once

#include "../../../prealloc/module.hpp"
#include "../../../prealloc/prepared_object_traversal.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::riscv::codegen {

struct RiscvPreparedObjectModuleResult;
struct RiscvPreparedObjectImageResult;

struct RiscvPreparedModuleFunctionAdmission {
  const c4c::backend::prepare::PreparedControlFlowFunction* control_flow = nullptr;
  std::string_view function_name;
  bool skip = false;
  std::optional<c4c::backend::prepare::PreparedObjectConsumerDiagnosticCategory>
      prepared_consumer_category;
  std::string diagnostic;

  [[nodiscard]] bool ok_to_emit() const {
    return control_flow != nullptr && !skip &&
           !prepared_consumer_category.has_value() && diagnostic.empty();
  }
};

[[nodiscard]] RiscvPreparedObjectModuleResult make_rv64_prepared_module_rejection(
    std::string diagnostic);

[[nodiscard]] RiscvPreparedObjectImageResult make_rv64_prepared_image_rejection(
    std::string diagnostic);

[[nodiscard]] RiscvPreparedModuleFunctionAdmission
admit_rv64_prepared_module_function(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow);

[[nodiscard]] std::string emit_prepared_module_text(
    const c4c::backend::prepare::PreparedBirModule& module);

}  // namespace c4c::backend::riscv::codegen
