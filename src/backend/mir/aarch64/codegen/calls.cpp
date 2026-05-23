#include "calls_printing.hpp"
#include "calls.hpp"
#include "calls_common.hpp"
#include "calls_effects.hpp"
#include "calls_moves.hpp"
#include "calls_argument_sources.hpp"
#include "calls_byval_aggregates.hpp"
#include "calls_preservation.hpp"
#include "dispatch_lookup.hpp"
#include "float_ops.hpp"
#include "f128.hpp"
#include "machine_printer.hpp"
#include "memory.hpp"
#include "variadic.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

thread_local bool g_publish_prepared_call_preserve_effects = true;

[[nodiscard]] bool publish_prepared_call_preserve_effects() {
  return g_publish_prepared_call_preserve_effects;
}

ScopedPreparedCallPreserveEffectPublication::ScopedPreparedCallPreserveEffectPublication(
    bool enabled)
    : previous_enabled_(g_publish_prepared_call_preserve_effects) {
  g_publish_prepared_call_preserve_effects = enabled;
}

ScopedPreparedCallPreserveEffectPublication::~ScopedPreparedCallPreserveEffectPublication() {
  g_publish_prepared_call_preserve_effects = previous_enabled_;
}


const prepare::PreparedCallPlan* find_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.call_plans == nullptr) {
    return nullptr;
  }
  const auto position_key = [](std::size_t block_index, std::size_t inst_index) {
    return (block_index << 32U) ^ inst_index;
  };
  if (context.function.call_plan_indexes != nullptr) {
    const auto it = context.function.call_plan_indexes->calls_by_position.find(
        position_key(context.block_index, instruction_index));
    return it == context.function.call_plan_indexes->calls_by_position.end()
               ? nullptr
               : it->second;
  }
  for (const auto& call : context.function.call_plans->calls) {
    if (call.block_index == context.block_index &&
        call.instruction_index == instruction_index) {
      return &call;
    }
  }
  return nullptr;
}

const prepare::PreparedCallPlan* require_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* call_plan = find_prepared_call_plan(context, instruction_index);
  if (call_plan == nullptr) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingPreparedCallPlan,
        context,
        instruction_index,
        "AArch64 call lowering requires an authoritative PreparedCallPlan");
  }
  return call_plan;
}


std::optional<module::MachineInstruction> lower_prepared_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan,
    const prepare::PreparedVariadicEntryHelperOperandHomes* variadic_helper_operand_homes,
    std::optional<prepare::PreparedVariadicEntryHelperKind> variadic_helper,
    module::ModuleLoweringDiagnostics& diagnostics) {
  CallInstructionRecord call_record{
      .wrapper_kind = call_plan.wrapper_kind,
      .variadic_fpr_arg_register_count = call_plan.variadic_fpr_arg_register_count,
      .memory_return = call_plan.memory_return,
      .memory_return_storage =
          call_plan.memory_return.has_value()
              ? make_memory_return_storage(context,
                                           *call_plan.memory_return,
                                           instruction_index)
              : std::nullopt,
      .prepared_indirect_callee = call_plan.indirect_callee,
      .prepared_arguments = call_plan.arguments,
      .prepared_result = call_plan.result,
      .preserved_values = publish_prepared_call_preserve_effects()
                              ? call_plan.preserved_values
                              : std::vector<prepare::PreparedCallPreservedValue>{},
      .clobbered_registers = call_plan.clobbered_registers,
      .source_call = &call_plan,
      .outgoing_stack_argument_bytes =
          outgoing_stack_argument_bytes(call_inst, call_plan),
      .source_variadic_entry = variadic_entry_plan,
      .source_variadic_helper_operand_homes = variadic_helper_operand_homes,
      .variadic_entry_helper = variadic_helper,
      .variadic_va_start_overflow_area_stack_offset_bytes =
          va_start_overflow_area_stack_offset(context, variadic_entry_plan, variadic_helper),
      .calling_convention = call_inst.calling_convention,
      .is_indirect = call_plan.is_indirect,
      .is_variadic =
          call_plan.wrapper_kind == prepare::PreparedCallWrapperKind::DirectExternVariadic ||
          call_inst.is_variadic,
      .is_noreturn = call_inst.is_noreturn,
  };

  if (call_plan.is_indirect) {
    if (!call_inst.is_indirect || !call_plan.indirect_callee.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 indirect call lowering requires matching retained BIR and prepared indirect callee facts");
      return std::nullopt;
    }
    auto callee = make_indirect_callee_register(
        context, *call_plan.indirect_callee, instruction_index, diagnostics);
    if (!callee.has_value()) {
      return std::nullopt;
    }
    call_record.indirect_callee = make_register_operand(*callee);
  } else {
    if (call_inst.is_indirect || !call_plan.direct_callee_name.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 direct call lowering requires matching retained BIR and prepared direct callee facts");
      return std::nullopt;
    }
    call_record.direct_callee = SymbolOperand{
        .link_name = call_inst.callee_link_name_id,
        .type = bir::TypeKind::Ptr,
        .is_extern = call_plan.wrapper_kind != prepare::PreparedCallWrapperKind::SameModule,
    };
    call_record.direct_callee_label = *call_plan.direct_callee_name;
  }

  InstructionRecord target = make_call_instruction(std::move(call_record));
  target.function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName;
  target.block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;

  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}


}  // namespace c4c::backend::aarch64::codegen
