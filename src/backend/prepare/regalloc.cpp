#include "regalloc.hpp"

// Execution note: this file is still a scaffold.
// Follow ref/claudes-c-compiler/src/backend/regalloc.rs for the intended
// register-allocation pipeline and policy shape.

namespace c4c::backend::prepare {

namespace {

std::string_view regalloc_allocation_kind(std::string_view contract_kind) {
  if (contract_kind == "address_exposed_storage") {
    return "fixed_stack_storage";
  }
  return "register_candidate";
}

}  // namespace

void run_regalloc(PreparedLirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("regalloc");
  module.notes.push_back(PrepareNote{
      .phase = "regalloc",
      .message = "regalloc skeleton is wired; no physical register assignment is performed yet",
  });
}

void run_regalloc(PreparedBirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("regalloc");
  module.regalloc.functions.clear();
  module.regalloc.functions.reserve(module.module.functions.size());

  for (const auto& function : module.module.functions) {
    PreparedRegallocFunction prepared_function{
        .function_name = function.name,
    };
    for (const auto& object : module.liveness.objects) {
      if (object.function_name != function.name) {
        continue;
      }
      const std::string allocation_kind(regalloc_allocation_kind(object.contract_kind));
      prepared_function.objects.push_back(PreparedRegallocObject{
          .function_name = object.function_name,
          .source_name = object.source_name,
          .source_kind = object.source_kind,
          .contract_kind = object.contract_kind,
          .allocation_kind = allocation_kind,
      });
      if (allocation_kind == "fixed_stack_storage") {
        ++prepared_function.fixed_stack_storage_count;
      } else {
        ++prepared_function.register_candidate_count;
      }
    }
    if (!prepared_function.objects.empty()) {
      module.regalloc.functions.push_back(std::move(prepared_function));
    }
  }

  module.notes.push_back(PrepareNote{
      .phase = "regalloc",
      .message =
          "regalloc now groups prepared liveness objects per function and classifies them as "
          "register_candidate or fixed_stack_storage contracts for downstream prepared-BIR "
          "consumers; physical register assignment remains future work",
  });
}

}  // namespace c4c::backend::prepare
