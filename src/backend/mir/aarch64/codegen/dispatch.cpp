#include "dispatch.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "atomics.hpp"
#include "calls.hpp"
#include "cast_ops.hpp"
#include "comparison.hpp"
#include "f128.hpp"
#include "float_ops.hpp"
#include "globals.hpp"
#include "i128_ops.hpp"
#include "inline_asm.hpp"
#include "intrinsics.hpp"
#include "memory.hpp"
#include "returns.hpp"
#include "variadic.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

void append_block_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                             module::ModuleLoweringDiagnosticKind kind,
                             const module::BlockLoweringContext& context,
                             std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = kind,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .message = std::move(message),
  });
}

[[nodiscard]] std::string unsupported_terminator_message(
    c4c::backend::bir::TerminatorKind kind) {
  switch (kind) {
    case c4c::backend::bir::TerminatorKind::Return:
      return "AArch64 block dispatch visited unsupported prepared return terminator";
    case c4c::backend::bir::TerminatorKind::Branch:
      return "AArch64 block dispatch visited prepared branch terminator; semantic lowering is not implemented";
    case c4c::backend::bir::TerminatorKind::CondBranch:
      return "AArch64 block dispatch visited prepared conditional branch terminator; semantic lowering is not implemented";
  }
  return "AArch64 block dispatch visited unsupported prepared terminator";
}

[[nodiscard]] const bir::Block* find_bir_block(
    const module::FunctionLoweringContext& function,
    const prepare::PreparedControlFlowBlock& block) {
  if (function.bir_function == nullptr) {
    return nullptr;
  }

  if (function.prepared == nullptr || block.block_label == c4c::kInvalidBlockLabel) {
    return nullptr;
  }

  const std::string_view prepared_block_label =
      prepare::prepared_block_label(function.prepared->names, block.block_label);
  if (prepared_block_label.empty()) {
    return nullptr;
  }

  for (const auto& bir_block : function.bir_function->blocks) {
    if (bir_block.label_id != c4c::kInvalidBlockLabel &&
        function.prepared->module.names.block_labels.spelling(bir_block.label_id) ==
            prepared_block_label) {
      return &bir_block;
    }
    if (std::string_view{bir_block.label} == prepared_block_label) {
      return &bir_block;
    }
  }
  return nullptr;
}

[[nodiscard]] module::InstructionLoweringFamily classify_instruction(
    const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> module::InstructionLoweringFamily {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::PhiInst>) {
          return module::InstructionLoweringFamily::Phi;
        } else if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                             std::is_same_v<T, bir::CastInst>) {
          return module::InstructionLoweringFamily::Scalar;
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          return module::InstructionLoweringFamily::Select;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return module::InstructionLoweringFamily::Call;
        } else if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                             std::is_same_v<T, bir::LoadGlobalInst> ||
                             std::is_same_v<T, bir::StoreLocalInst> ||
                             std::is_same_v<T, bir::StoreGlobalInst>) {
          return module::InstructionLoweringFamily::Memory;
        }
        return module::InstructionLoweringFamily::Unknown;
      },
      inst);
}

void append_unsupported_instruction_diagnostic(
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .instruction_family = classify_instruction(inst),
      .message =
          "AArch64 block dispatch visited unsupported prepared BIR instruction family",
  });
}

void append_call_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                            module::ModuleLoweringDiagnosticKind kind,
                            const module::BlockLoweringContext& context,
                            std::size_t instruction_index,
                            std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = kind,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .instruction_family = module::InstructionLoweringFamily::Call,
      .message = std::move(message),
  });
}

[[nodiscard]] module::MachineInstruction make_bir_machine_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    InstructionRecord target) {
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

[[nodiscard]] std::optional<module::MachineInstruction> lower_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto variadic_helper = variadic_entry_helper_kind(call_inst.callee);
  const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan = nullptr;
  if (variadic_helper.has_value()) {
    variadic_entry_plan =
        require_prepared_variadic_entry_plan(context, instruction_index, diagnostics);
    if (variadic_entry_plan == nullptr) {
      return std::nullopt;
    }
  }

  const auto* call_plan =
      require_prepared_call_plan(context, instruction_index, diagnostics);
  if (call_plan == nullptr) {
    return std::nullopt;
  }

  const prepare::PreparedVariadicEntryHelperOperandHomes* variadic_helper_operand_homes =
      nullptr;
  if (variadic_entry_plan != nullptr) {
    variadic_helper_operand_homes =
        prepare::find_prepared_variadic_entry_helper_operand_homes(
            *variadic_entry_plan, context.block_index, instruction_index);
    if (variadic_helper_operand_homes == nullptr ||
        !variadic_helper_operand_homes_complete(*variadic_helper_operand_homes)) {
      std::string message =
          "AArch64 variadic entry helper lowering requires prepared helper operand-home facts";
      const auto missing_consumption_fact =
          variadic_helper_missing_consumption_fact_message(*variadic_helper);
      if (!missing_consumption_fact.empty()) {
        message = missing_consumption_fact;
      }
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          std::move(message));
      return std::nullopt;
    }
  }

  return lower_prepared_call_instruction(context,
                                         call_inst,
                                         *call_plan,
                                         instruction_index,
                                         variadic_entry_plan,
                                         variadic_helper_operand_homes,
                                         variadic_helper,
                                         diagnostics);
}

}  // namespace

module::BlockLoweringContext make_block_lowering_context(
    module::FunctionLoweringContext function,
    const prepare::PreparedControlFlowBlock& block,
    std::size_t block_index) {
  return module::BlockLoweringContext{
      .function = function,
      .control_flow_block = &block,
      .bir_block = find_bir_block(function, block),
      .block_index = block_index,
  };
}

InstructionDispatchResult dispatch_prepared_block(
    const module::BlockLoweringContext& context,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics) {
  InstructionDispatchResult result;

  if (context.function.control_flow == nullptr || context.control_flow_block == nullptr) {
    append_block_diagnostic(diagnostics,
                            module::ModuleLoweringDiagnosticKind::MissingBlockContext,
                            context,
                            "AArch64 block dispatch requires prepared function and block context");
    return result;
  }

  block.block_label = context.control_flow_block->block_label;
  block.index = context.block_index;
  block.successors.clear();

  if (context.bir_block == nullptr && context.function.bir_function != nullptr) {
    append_block_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingInstructionBlockMapping,
        context,
        "AArch64 block dispatch could not map prepared block to retained BIR instructions");
  }

  BlockScalarLoweringState scalar_state;
  if (context.bir_block != nullptr) {
    for (std::size_t instruction_index = 0;
         instruction_index < context.bir_block->insts.size();
         ++instruction_index) {
      const auto& inst = context.bir_block->insts[instruction_index];
      if (const auto* call = std::get_if<bir::CallInst>(&inst)) {
        if (call->inline_asm.has_value() ||
            has_prepared_inline_asm_carrier(context, instruction_index)) {
          if (auto lowered = lower_inline_asm_instruction(
                  context, *call, instruction_index, diagnostics)) {
            block.instructions.push_back(std::move(*lowered));
          }
          ++result.visited_operations;
          continue;
        }
        if (has_prepared_intrinsic_carrier(context, instruction_index)) {
          if (auto lowered = lower_intrinsic_instruction(
                  context, instruction_index, diagnostics)) {
            block.instructions.push_back(std::move(*lowered));
          }
          ++result.visited_operations;
          continue;
        }
        const auto* call_plan = find_prepared_call_plan(context, instruction_index);
        if (call_plan != nullptr) {
          auto before_call_moves =
              lower_before_call_moves(context, *call_plan, instruction_index, diagnostics);
          for (auto& before_call_move : before_call_moves) {
            block.instructions.push_back(std::move(before_call_move));
          }
        }
        if (auto lowered = lower_call_instruction(
                context, *call, instruction_index, diagnostics)) {
          block.instructions.push_back(std::move(*lowered));
          if (call_plan != nullptr) {
            auto after_call_moves =
                lower_after_call_moves(context, *call_plan, instruction_index, diagnostics);
            for (auto& after_call_move : after_call_moves) {
              block.instructions.push_back(std::move(after_call_move));
            }
          }
        }
      } else if (auto lowered = lower_address_materialization(
                     context, instruction_index, diagnostics)) {
        if (const auto* address_record =
                std::get_if<AddressMaterializationRecord>(&lowered->target.payload);
            address_record != nullptr && address_record->result_register.has_value()) {
          record_emitted_scalar_register(scalar_state,
                                         address_record->result_value_name,
                                         *address_record->result_register);
        }
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered_i128_pair =
                     lower_i128_pair_operation_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_i128_pair.handled) {
        if (lowered_i128_pair.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_i128_pair.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered_i128_copy =
                     lower_i128_copy_instruction(context, inst, instruction_index, diagnostics);
                 lowered_i128_copy.handled) {
        if (lowered_i128_copy.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_i128_copy.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered_f128_helper =
                     lower_f128_runtime_helper_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_f128_helper.handled) {
        if (lowered_f128_helper.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_f128_helper.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered = lower_prepared_scalar_float_alu_instruction(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_cast_instruction(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_instruction(
              context, inst, instruction_index, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(*lowered));
      } else {
        auto lowered_i128_transport =
            lower_i128_transport_instruction(context, inst, instruction_index, diagnostics);
        if (lowered_i128_transport.handled) {
          if (lowered_i128_transport.instruction.has_value()) {
            block.instructions.push_back(std::move(*lowered_i128_transport.instruction));
          }
          ++result.visited_operations;
          continue;
        }
        auto lowered_memory =
            lower_f128_transport_instruction(context, inst, instruction_index, diagnostics);
        if (lowered_memory.handled) {
          if (lowered_memory.instruction.has_value()) {
            if (const auto* memory_record =
                    std::get_if<MemoryInstructionRecord>(
                        &lowered_memory.instruction->target.payload);
                memory_record != nullptr && memory_record->result_register.has_value()) {
              record_emitted_scalar_register(scalar_state,
                                             memory_record->result_value_name,
                                             *memory_record->result_register);
            }
            block.instructions.push_back(std::move(*lowered_memory.instruction));
          }
        } else if (auto lowered_ordinary_memory =
                       lower_memory_instruction(context, inst, instruction_index, diagnostics);
                   lowered_ordinary_memory.handled) {
          if (lowered_ordinary_memory.instruction.has_value()) {
            if (const auto* memory_record =
                    std::get_if<MemoryInstructionRecord>(
                        &lowered_ordinary_memory.instruction->target.payload);
                memory_record != nullptr && memory_record->result_register.has_value()) {
              record_emitted_scalar_register(scalar_state,
                                             memory_record->result_value_name,
                                             *memory_record->result_register);
            }
            block.instructions.push_back(std::move(*lowered_ordinary_memory.instruction));
          }
        } else {
          append_unsupported_instruction_diagnostic(
              diagnostics, context, inst, instruction_index);
        }
      }
      ++result.visited_operations;
    }
  }

  auto lowered_atomic_operations =
      lower_atomic_memory_operations_for_block(context, diagnostics);
  result.visited_operations += lowered_atomic_operations.size();
  for (auto& atomic_instruction : lowered_atomic_operations) {
    block.instructions.push_back(std::move(atomic_instruction));
  }

  result.visited_terminator = true;
  if (context.control_flow_block->terminator_kind ==
      c4c::backend::bir::TerminatorKind::Return) {
    if (auto lowered =
            lower_prepared_return_terminator(context, scalar_state, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::Branch) {
    if (auto lowered = lower_prepared_branch_terminator(context, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors.push_back(make_unconditional_branch_successor(context));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::CondBranch) {
    if (auto lowered =
            lower_prepared_conditional_branch_terminator(context, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    }
  } else {
    append_block_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily,
        context,
        unsupported_terminator_message(context.control_flow_block->terminator_kind));
  }

  result.emitted_instructions = block.instructions.size();
  return result;
}

}  // namespace c4c::backend::aarch64::codegen
