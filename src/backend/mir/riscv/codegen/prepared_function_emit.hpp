#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/module.hpp"
#include "../../../prealloc/prepared_lookups.hpp"

#include <cstddef>
#include <optional>
#include <string>

namespace c4c::backend::riscv::codegen {

struct RiscvPreparedFunctionAdmissionCallbacks {
  std::optional<std::string> (*variadic_function_admission_diagnostic)(
      const c4c::backend::prepare::PreparedBirModule& prepared,
      c4c::FunctionNameId function_name) = nullptr;
  std::optional<std::size_t> (*stack_frame_size)(
      const c4c::backend::prepare::PreparedAddressingFunction* addressing,
      const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
      const c4c::backend::prepare::PreparedStackLayout& stack_layout) = nullptr;
  std::optional<std::string> (*saved_register_bank_diagnostic)(
      const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan) = nullptr;
  std::optional<std::string> (*param_homes_diagnostic)(
      const c4c::backend::prepare::PreparedStackLayout& stack_layout,
      const c4c::backend::prepare::PreparedNameTables& names,
      const c4c::backend::prepare::PreparedFunctionLookups* lookups,
      const c4c::backend::prepare::PreparedValueLocationFunction* value_locations,
      const c4c::backend::bir::Function& function,
      std::size_t stack_frame_bytes) = nullptr;
  std::optional<std::string> (*variadic_helper_diagnostic)(
      const c4c::backend::prepare::PreparedBirModule& prepared,
      c4c::FunctionNameId function_name,
      const c4c::backend::bir::Function& function,
      std::size_t stack_frame_bytes) = nullptr;
};

struct RiscvPreparedFunctionAdmissionResult {
  const c4c::backend::bir::Function* function = nullptr;
  std::string function_name;
  c4c::backend::prepare::PreparedFunctionLookups lookups;
  c4c::backend::prepare::PreparedDependencyOperandAuthorityRecords
      dependency_operand_authorities;
  c4c::backend::prepare::PreparedSelectCarrierAliasAuthorityRecords
      carrier_alias_authorities;
  c4c::backend::prepare::PreparedSelectEdgeSourceProducerPlacementRecords
      select_edge_source_producer_placements;
  const c4c::backend::prepare::PreparedAddressingFunction* addressing = nullptr;
  const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan = nullptr;
  const c4c::backend::prepare::PreparedStoragePlanFunction* storage_plan = nullptr;
  const c4c::backend::prepare::PreparedValueLocationFunction* value_locations = nullptr;
  const c4c::backend::prepare::PreparedInlineAsmCarrierFunction*
      inline_asm_carriers = nullptr;
  std::optional<std::size_t> stack_frame_bytes;
  bool has_call = false;
  std::string diagnostic;
};

[[nodiscard]] RiscvPreparedFunctionAdmissionResult
prepare_rv64_object_function_admission_shell(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const RiscvPreparedFunctionAdmissionCallbacks& callbacks);

[[nodiscard]] bool append_simple_prepared_bir_function_asm(
    std::string& out,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Function& function);

}  // namespace c4c::backend::riscv::codegen
