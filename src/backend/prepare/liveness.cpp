#include "liveness.hpp"

// Execution note: this file is still a scaffold.
// Follow ref/claudes-c-compiler/src/backend/liveness.rs for the intended
// liveness/dataflow analysis shape.

namespace c4c::backend::prepare {

namespace {

std::string_view liveness_contract_kind(std::string_view source_kind) {
  if (source_kind == "address_taken_local_slot" || source_kind == "sret_param" ||
      source_kind == "byval_param" || source_kind == "call_result_sret" ||
      source_kind == "va_arg_aggregate_result") {
    return "address_exposed_storage";
  }
  return "value_storage";
}

}  // namespace

void run_liveness(PreparedLirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("liveness");
  module.notes.push_back(PrepareNote{
      .phase = "liveness",
      .message = "liveness skeleton is wired; no live-interval computation is performed yet",
  });
}

void run_liveness(PreparedBirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("liveness");
  module.liveness.objects.clear();
  module.liveness.objects.reserve(module.stack_layout.objects.size());

  for (const auto& object : module.stack_layout.objects) {
    module.liveness.objects.push_back(PreparedLivenessObject{
        .function_name = object.function_name,
        .source_name = object.source_name,
        .source_kind = object.source_kind,
        .contract_kind = std::string(liveness_contract_kind(object.source_kind)),
    });
  }

  module.notes.push_back(PrepareNote{
      .phase = "liveness",
      .message =
          "liveness now classifies prepared stack-layout objects into value-storage "
          "or address-exposed storage contracts for downstream prepared-BIR "
          "consumers; live-interval computation remains future work",
  });
}

}  // namespace c4c::backend::prepare
