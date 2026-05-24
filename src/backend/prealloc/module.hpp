#pragma once

#include "addressing.hpp"
#include "calls.hpp"
#include "control_flow.hpp"
#include "decoded_home_storage.hpp"
#include "formal_publications.hpp"
#include "frame.hpp"
#include "liveness.hpp"
#include "names.hpp"
#include "regalloc.hpp"
#include "runtime_helpers.hpp"
#include "special_carriers.hpp"
#include "storage.hpp"
#include "value_locations.hpp"
#include "variadic.hpp"
#include "prepared_lookups.hpp"
#include "publication_plans.hpp"

#include "../bir/bir.hpp"
#include "../../target_profile.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::prepare {

[[nodiscard]] std::optional<bir::CallArgAbiInfo> infer_call_arg_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind type);

namespace stack_layout {

struct FunctionInlineAsmSummary {
  std::size_t instruction_count = 0;
  bool has_side_effects = false;
};

std::vector<PreparedStackObject> collect_function_stack_objects(PreparedNameTables& names,
                                                                const bir::NameTables& bir_names,
                                                                const bir::Function& function,
                                                                PreparedObjectId& next_object_id);

void apply_alloca_coalescing_hints(const PreparedNameTables& names,
                                   const bir::Function& function,
                                   std::vector<PreparedStackObject>& objects);

void apply_copy_coalescing_hints(const PreparedNameTables& names,
                                 const bir::Function& function,
                                 std::vector<PreparedStackObject>& objects);

void apply_aggregate_address_publication_hints(const PreparedNameTables& names,
                                               const bir::Function& function,
                                               std::vector<PreparedStackObject>& objects);

FunctionInlineAsmSummary summarize_inline_asm(const bir::Function& function);

void apply_regalloc_hints(PreparedNameTables& names,
                          const bir::Function& function,
                          const FunctionInlineAsmSummary& inline_asm_summary,
                          std::vector<PreparedStackObject>& objects);

std::vector<PreparedFrameSlot> assign_frame_slots(const PreparedNameTables& names,
                                                  const std::vector<PreparedStackObject>& objects,
                                                  PreparedFrameSlotId& next_slot_id,
                                                  std::size_t& frame_size_bytes,
                                                  std::size_t& frame_alignment_bytes);

}  // namespace stack_layout

struct PreparedBirModule {
  c4c::backend::bir::Module module;
  c4c::TargetProfile target_profile{};
  PrepareRoute route = PrepareRoute::SemanticBirShared;
  std::vector<PreparedBirInvariant> invariants;
  PreparedNameTables names;
  PreparedControlFlow control_flow;
  PreparedValueLocations value_locations;
  PreparedStackLayout stack_layout;
  PreparedAddressing addressing;
  PreparedLiveness liveness;
  PreparedRegisterGroupOverrides register_group_overrides;
  PreparedRegalloc regalloc;
  PreparedFramePlan frame_plan;
  PreparedDynamicStackPlan dynamic_stack_plan;
  PreparedCallPlans call_plans;
  PreparedVariadicEntryPlans variadic_entry_plans;
  PreparedStoragePlans storage_plans;
  PreparedI128Carriers i128_carriers;
  PreparedF128Carriers f128_carriers;
  PreparedAtomicOperations atomic_operations;
  PreparedIntrinsicCarriers intrinsic_carriers;
  PreparedInlineAsmCarriers inline_asm_carriers;
  PreparedF128RuntimeHelpers f128_runtime_helpers;
  PreparedI128RuntimeHelpers i128_runtime_helpers;
  std::vector<std::string> completed_phases;
  std::vector<PrepareNote> notes;
};

[[nodiscard]] inline const PreparedRegisterGroupOverride* find_prepared_register_group_override(
    const PreparedRegisterGroupOverrides& overrides,
    FunctionNameId function_name,
    ValueNameId value_name) {
  for (const auto& override : overrides.values) {
    if (override.function_name == function_name && override.value_name == value_name) {
      return &override;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedRegisterGroupOverride* find_prepared_register_group_override(
    const PreparedBirModule& module,
    FunctionNameId function_name,
    ValueNameId value_name) {
  return find_prepared_register_group_override(module.register_group_overrides,
                                               function_name,
                                               value_name);
}

[[nodiscard]] inline const PreparedAddressingFunction* find_prepared_addressing(
    const PreparedBirModule& module,
    FunctionNameId function_name) {
  return find_prepared_addressing_function(module.addressing, function_name);
}

[[nodiscard]] inline const PreparedValueLocationFunction* find_prepared_value_location_function(
    const PreparedBirModule& module,
    FunctionNameId function_name) {
  return find_prepared_value_location_function(module.value_locations, function_name);
}

[[nodiscard]] inline const PreparedValueLocationFunction* find_prepared_value_location_function(
    const PreparedBirModule& module,
    std::string_view function_name) {
  return find_prepared_value_location_function(module.names, module.value_locations, function_name);
}

[[nodiscard]] inline const PreparedFramePlanFunction* find_prepared_frame_plan(
    const PreparedFramePlan& frame_plan,
    FunctionNameId function_name) {
  for (const auto& function_plan : frame_plan.functions) {
    if (function_plan.function_name == function_name) {
      return &function_plan;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedFramePlanFunction* find_prepared_frame_plan(
    const PreparedBirModule& module,
    FunctionNameId function_name) {
  return find_prepared_frame_plan(module.frame_plan, function_name);
}

[[nodiscard]] inline const PreparedDynamicStackPlanFunction* find_prepared_dynamic_stack_plan(
    const PreparedDynamicStackPlan& dynamic_stack_plan,
    FunctionNameId function_name) {
  for (const auto& function_plan : dynamic_stack_plan.functions) {
    if (function_plan.function_name == function_name) {
      return &function_plan;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedDynamicStackPlanFunction* find_prepared_dynamic_stack_plan(
    const PreparedBirModule& module,
    FunctionNameId function_name) {
  return find_prepared_dynamic_stack_plan(module.dynamic_stack_plan, function_name);
}

[[nodiscard]] inline const PreparedCallPlansFunction* find_prepared_call_plans(
    const PreparedCallPlans& call_plans,
    FunctionNameId function_name) {
  for (const auto& function_plan : call_plans.functions) {
    if (function_plan.function_name == function_name) {
      return &function_plan;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedCallPlansFunction* find_prepared_call_plans(
    const PreparedBirModule& module,
    FunctionNameId function_name) {
  return find_prepared_call_plans(module.call_plans, function_name);
}

[[nodiscard]] inline const PreparedVariadicEntryPlanFunction*
find_prepared_variadic_entry_plan(const PreparedVariadicEntryPlans& variadic_entry_plans,
                                  FunctionNameId function_name) {
  for (const auto& function_plan : variadic_entry_plans.functions) {
    if (function_plan.function_name == function_name) {
      return &function_plan;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedVariadicEntryPlanFunction*
find_prepared_variadic_entry_plan(const PreparedBirModule& module,
                                  FunctionNameId function_name) {
  return find_prepared_variadic_entry_plan(module.variadic_entry_plans, function_name);
}

[[nodiscard]] inline const PreparedVariadicEntryHelperOperandHomes*
find_prepared_variadic_entry_helper_operand_homes(
    const PreparedVariadicEntryPlanFunction& function_plan,
    std::size_t block_index,
    std::size_t instruction_index) {
  for (const auto& homes : function_plan.helper_operand_homes) {
    if (homes.block_index == block_index &&
        homes.instruction_index == instruction_index) {
      return &homes;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedStoragePlanFunction* find_prepared_storage_plan(
    const PreparedStoragePlans& storage_plans,
    FunctionNameId function_name) {
  for (const auto& function_plan : storage_plans.functions) {
    if (function_plan.function_name == function_name) {
      return &function_plan;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedStoragePlanFunction* find_prepared_storage_plan(
    const PreparedBirModule& module,
    FunctionNameId function_name) {
  return find_prepared_storage_plan(module.storage_plans, function_name);
}

[[nodiscard]] inline const PreparedI128CarrierFunction* find_prepared_i128_carriers(
    const PreparedI128Carriers& carriers,
    FunctionNameId function_name) {
  for (const auto& function_carriers : carriers.functions) {
    if (function_carriers.function_name == function_name) {
      return &function_carriers;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedI128CarrierFunction* find_prepared_i128_carriers(
    const PreparedBirModule& module,
    FunctionNameId function_name) {
  return find_prepared_i128_carriers(module.i128_carriers, function_name);
}

[[nodiscard]] inline const PreparedF128CarrierFunction* find_prepared_f128_carriers(
    const PreparedF128Carriers& carriers,
    FunctionNameId function_name) {
  for (const auto& function_carriers : carriers.functions) {
    if (function_carriers.function_name == function_name) {
      return &function_carriers;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedF128CarrierFunction* find_prepared_f128_carriers(
    const PreparedBirModule& module,
    FunctionNameId function_name) {
  return find_prepared_f128_carriers(module.f128_carriers, function_name);
}

[[nodiscard]] inline const PreparedAtomicOperationFunction*
find_prepared_atomic_operations(const PreparedAtomicOperations& operations,
                                FunctionNameId function_name) {
  for (const auto& function_operations : operations.functions) {
    if (function_operations.function_name == function_name) {
      return &function_operations;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedAtomicOperationFunction*
find_prepared_atomic_operations(const PreparedBirModule& module,
                                FunctionNameId function_name) {
  return find_prepared_atomic_operations(module.atomic_operations, function_name);
}

[[nodiscard]] inline const PreparedIntrinsicCarrierFunction*
find_prepared_intrinsic_carriers(const PreparedIntrinsicCarriers& carriers,
                                 FunctionNameId function_name) {
  for (const auto& function_carriers : carriers.functions) {
    if (function_carriers.function_name == function_name) {
      return &function_carriers;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedIntrinsicCarrierFunction*
find_prepared_intrinsic_carriers(const PreparedBirModule& module,
                                 FunctionNameId function_name) {
  return find_prepared_intrinsic_carriers(module.intrinsic_carriers, function_name);
}

[[nodiscard]] inline const PreparedInlineAsmCarrierFunction*
find_prepared_inline_asm_carriers(const PreparedInlineAsmCarriers& carriers,
                                  FunctionNameId function_name) {
  for (const auto& function_carriers : carriers.functions) {
    if (function_carriers.function_name == function_name) {
      return &function_carriers;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedInlineAsmCarrierFunction*
find_prepared_inline_asm_carriers(const PreparedBirModule& module,
                                  FunctionNameId function_name) {
  return find_prepared_inline_asm_carriers(module.inline_asm_carriers, function_name);
}

[[nodiscard]] inline const PreparedI128RuntimeHelperFunction*
find_prepared_i128_runtime_helpers(const PreparedI128RuntimeHelpers& helpers,
                                   FunctionNameId function_name) {
  for (const auto& function_helpers : helpers.functions) {
    if (function_helpers.function_name == function_name) {
      return &function_helpers;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedI128RuntimeHelperFunction*
find_prepared_i128_runtime_helpers(const PreparedBirModule& module,
                                   FunctionNameId function_name) {
  return find_prepared_i128_runtime_helpers(module.i128_runtime_helpers, function_name);
}

[[nodiscard]] inline const PreparedF128RuntimeHelperFunction*
find_prepared_f128_runtime_helpers(const PreparedF128RuntimeHelpers& helpers,
                                   FunctionNameId function_name) {
  for (const auto& function_helpers : helpers.functions) {
    if (function_helpers.function_name == function_name) {
      return &function_helpers;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedF128RuntimeHelperFunction*
find_prepared_f128_runtime_helpers(const PreparedBirModule& module,
                                   FunctionNameId function_name) {
  return find_prepared_f128_runtime_helpers(module.f128_runtime_helpers, function_name);
}

[[nodiscard]] inline const PreparedI128Carrier* find_prepared_i128_carrier(
    const PreparedI128CarrierFunction& function_carriers,
    PreparedValueId value_id) {
  for (const auto& carrier : function_carriers.carriers) {
    if (carrier.value_id == value_id) {
      return &carrier;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedI128Carrier* find_prepared_i128_carrier(
    const PreparedI128CarrierFunction& function_carriers,
    ValueNameId value_name) {
  for (const auto& carrier : function_carriers.carriers) {
    if (carrier.value_name == value_name) {
      return &carrier;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedF128Carrier* find_prepared_f128_carrier(
    const PreparedF128CarrierFunction& function_carriers,
    PreparedValueId value_id) {
  for (const auto& carrier : function_carriers.carriers) {
    if (carrier.value_id == value_id) {
      return &carrier;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedF128Carrier* find_prepared_f128_carrier(
    const PreparedF128CarrierFunction& function_carriers,
    ValueNameId value_name) {
  for (const auto& carrier : function_carriers.carriers) {
    if (carrier.value_name == value_name) {
      return &carrier;
    }
  }
  return nullptr;
}

class BirPreAlloc {
 public:
  BirPreAlloc(const c4c::backend::bir::Module& module,
              const c4c::TargetProfile& target_profile,
              const PrepareOptions& options = {})
      : options_(options),
        prepared_{
            .module = module,
            .target_profile = target_profile,
            .route = PrepareRoute::SemanticBirShared,
            .completed_phases = {},
            .notes = {},
        } {}
  explicit BirPreAlloc(PreparedBirModule prepared,
                       const PrepareOptions& options = {})
      : options_(options), prepared_(std::move(prepared)) {
    if (prepared_.target_profile.arch == c4c::TargetArch::Unknown) {
      const auto resolved_triple =
          prepared_.module.target_triple.empty() ? c4c::default_host_target_triple()
                                                 : prepared_.module.target_triple;
      prepared_.target_profile = c4c::target_profile_from_triple(resolved_triple);
    }
  }

  void run_legalize();
  void run_stack_layout();
  void run_liveness();
  void run_out_of_ssa();
  void run_regalloc();
  void publish_contract_plans();

  PreparedBirModule& prepared() { return prepared_; }
  const PreparedBirModule& prepared() const { return prepared_; }
  PreparedBirModule run();

 private:
  void note(std::string_view message);

  PrepareOptions options_;
  PreparedBirModule prepared_;
};

PreparedBirModule prepare_semantic_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    const c4c::TargetProfile& target_profile,
    const PrepareOptions& options = {});

PreparedBirModule prepare_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    const c4c::TargetProfile& target_profile,
    const PrepareOptions& options = {});

}  // namespace c4c::backend::prepare
