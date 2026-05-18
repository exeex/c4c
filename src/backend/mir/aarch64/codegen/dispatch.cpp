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

[[nodiscard]] bool binary_result_matches_value(const bir::Inst& inst,
                                               std::string_view value_name) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  return binary != nullptr &&
         binary->result.kind == bir::Value::Kind::Named &&
         binary->result.name == value_name;
}

[[nodiscard]] bool binary_uses_named_value(const bir::Inst& inst,
                                           std::string_view value_name) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr) {
    return false;
  }
  const auto matches = [&](const bir::Value& value) {
    return value.kind == bir::Value::Kind::Named && value.name == value_name;
  };
  return matches(binary->lhs) || matches(binary->rhs);
}

[[nodiscard]] bool prepared_edge_select_source_is_destination_register(
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedValueHome& destination_home) {
  return source_home.kind == prepare::PreparedValueHomeKind::Register &&
         destination_home.kind == prepare::PreparedValueHomeKind::Register &&
         source_home.register_name.has_value() &&
         destination_home.register_name.has_value() &&
         *source_home.register_name == *destination_home.register_name;
}

[[nodiscard]] std::vector<module::MachineInstruction>
lower_predecessor_select_parallel_copy_sources(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.bir_function == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::Branch ||
      context.bir_block->terminator.kind != bir::TerminatorKind::Branch) {
    return lowered;
  }

  const auto* bundle = prepare::find_prepared_move_bundle(
      *context.function.value_locations,
      prepare::PreparedMovePhase::BlockEntry,
      context.block_index,
      0);
  if (bundle == nullptr ||
      bundle->authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
      bundle->source_parallel_copy_predecessor_label !=
          std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label} ||
      !bundle->source_parallel_copy_successor_label.has_value()) {
    return lowered;
  }

  const auto successor_label = prepare::prepared_block_label(
      context.function.prepared->names,
      *bundle->source_parallel_copy_successor_label);
  if (successor_label.empty() ||
      successor_label != context.bir_block->terminator.target_label) {
    return lowered;
  }
  const auto* successor =
      prepare::find_block_in_function(*context.function.bir_function, successor_label);
  if (successor == nullptr) {
    return lowered;
  }

  for (const auto& move : bundle->moves) {
    if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
        move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
        move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        move.source_immediate_i32.has_value() ||
        move.from_value_id == move.to_value_id) {
      continue;
    }
    const auto* source_home =
        prepare::find_prepared_value_home(*context.function.value_locations,
                                          move.from_value_id);
    const auto* destination_home =
        prepare::find_prepared_value_home(*context.function.value_locations,
                                          move.to_value_id);
    if (source_home == nullptr ||
        destination_home == nullptr ||
        source_home->value_name == c4c::kInvalidValueName ||
        destination_home->kind != prepare::PreparedValueHomeKind::Register) {
      continue;
    }
    const auto source_name =
        prepare::prepared_value_name(context.function.prepared->names, source_home->value_name);
    if (source_name.empty()) {
      continue;
    }
    if (!prepared_edge_select_source_is_destination_register(*source_home,
                                                            *destination_home)) {
      continue;
    }
    for (std::size_t source_index = 0; source_index < successor->insts.size(); ++source_index) {
      if (!binary_result_matches_value(successor->insts[source_index], source_name)) {
        continue;
      }
      auto edge_context = context;
      edge_context.bir_block = successor;
      BlockScalarLoweringState edge_state = scalar_state;
      if (source_index > 0) {
        const auto& previous = successor->insts[source_index - 1];
        const auto* previous_binary = std::get_if<bir::BinaryInst>(&previous);
        if (previous_binary != nullptr &&
            previous_binary->result.kind == bir::Value::Kind::Named &&
            binary_uses_named_value(successor->insts[source_index],
                                    previous_binary->result.name)) {
          if (auto previous_lowered =
                  lower_scalar_instruction(edge_context,
                                           previous,
                                           source_index - 1,
                                           edge_state,
                                           diagnostics)) {
            lowered.push_back(std::move(*previous_lowered));
          }
        }
      }
      auto source_lowered =
          lower_scalar_control_value_instruction(edge_context,
                                                 successor->insts[source_index],
                                                 source_index,
                                                 edge_state,
                                                 diagnostics);
      if (!source_lowered.has_value()) {
        lowered.clear();
        return lowered;
      }
      lowered.push_back(std::move(*source_lowered));
      scalar_state = std::move(edge_state);
      return lowered;
    }
  }
  return lowered;
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

[[nodiscard]] bool is_dynamic_alloca_helper(std::string_view callee) {
  constexpr std::string_view kPrefix = "llvm.dynamic_alloca.";
  return callee.substr(0, kPrefix.size()) == kPrefix;
}

[[nodiscard]] std::optional<prepare::PreparedDynamicStackOpKind>
dynamic_stack_helper_kind(std::string_view callee) {
  if (callee == "llvm.stacksave") {
    return prepare::PreparedDynamicStackOpKind::StackSave;
  }
  if (is_dynamic_alloca_helper(callee)) {
    return prepare::PreparedDynamicStackOpKind::DynamicAlloca;
  }
  if (callee == "llvm.stackrestore") {
    return prepare::PreparedDynamicStackOpKind::StackRestore;
  }
  return std::nullopt;
}

[[nodiscard]] const prepare::PreparedDynamicStackOp* find_dynamic_stack_op(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    prepare::PreparedDynamicStackOpKind kind) {
  if (context.function.dynamic_stack_plan == nullptr) {
    return nullptr;
  }
  const c4c::BlockLabelId block_label =
      context.control_flow_block != nullptr ? context.control_flow_block->block_label
                                            : c4c::kInvalidBlockLabel;
  for (const auto& op : context.function.dynamic_stack_plan->operations) {
    if (op.kind == kind && op.instruction_index == instruction_index &&
        (op.block_label == block_label || op.block_label == c4c::kInvalidBlockLabel ||
         block_label == c4c::kInvalidBlockLabel)) {
      return &op;
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedValueHome* find_dynamic_stack_value_home(
    const module::FunctionLoweringContext& context,
    c4c::ValueNameId value_name) {
  if (context.value_locations == nullptr || value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  for (const auto& home : context.value_locations->value_homes) {
    if (home.value_name == value_name) {
      return &home;
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedValueHome* find_dynamic_stack_value_home(
    const module::FunctionLoweringContext& context,
    const std::optional<c4c::ValueNameId>& value_name) {
  if (!value_name.has_value()) {
    return nullptr;
  }
  return find_dynamic_stack_value_home(context, *value_name);
}

[[nodiscard]] std::optional<std::string> dynamic_stack_register_home_name(
    const prepare::PreparedValueHome& home) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value() || home.register_name->empty()) {
    return std::nullopt;
  }
  return home.register_name;
}

[[nodiscard]] std::optional<std::string> dynamic_stack_stack_home_address(
    const module::FunctionLoweringContext& context,
    const prepare::PreparedValueHome& home) {
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.offset_bytes.has_value()) {
    return std::nullopt;
  }
  if (context.frame_plan == nullptr ||
      !context.frame_plan->uses_frame_pointer_for_fixed_slots) {
    return std::nullopt;
  }
  return std::string{"[x29, #"} + std::to_string(*home.offset_bytes) + "]";
}

[[nodiscard]] bool dynamic_stack_home_requires_stable_frame_pointer(
    const module::FunctionLoweringContext& context,
    const prepare::PreparedValueHome& home) {
  return home.kind == prepare::PreparedValueHomeKind::StackSlot &&
         (context.frame_plan == nullptr ||
          !context.frame_plan->uses_frame_pointer_for_fixed_slots);
}

[[nodiscard]] std::optional<std::size_t> dynamic_stack_element_size_bytes(
    const prepare::PreparedDynamicStackOp& op) {
  if (op.element_size_bytes != 0) {
    return op.element_size_bytes;
  }
  if (op.allocation_type_text == "i8") {
    return std::size_t{1};
  }
  if (op.allocation_type_text == "i16") {
    return std::size_t{2};
  }
  if (op.allocation_type_text == "i32") {
    return std::size_t{4};
  }
  if (op.allocation_type_text == "i64" || op.allocation_type_text == "ptr") {
    return std::size_t{8};
  }
  return std::nullopt;
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

[[nodiscard]] InstructionRecord make_dynamic_stack_rejection_record(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::string_view diagnostic) {
  return InstructionRecord{
      .family = InstructionFamily::Frame,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::FrameSetup,
      .selection =
          MachineNodeStatusRecord{
              .status = MachineNodeSelectionStatus::DeferredUnsupported,
              .diagnostic = diagnostic,
          },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::FrameSetup},
      .payload = FrameInstructionRecord{},
  };
}

[[nodiscard]] module::MachineInstruction make_dynamic_stack_assembler_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::vector<std::string> lines) {
  std::string asm_text;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += lines[index];
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::FrameSetup},
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_bir_machine_instruction(context, instruction_index, std::move(target));
}

[[nodiscard]] std::optional<module::MachineInstruction> make_dynamic_stack_failure(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::string_view message,
    module::ModuleLoweringDiagnostics& diagnostics) {
  append_call_diagnostic(diagnostics,
                         module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
                         context,
                         instruction_index,
                         std::string{message});
  return make_bir_machine_instruction(
      context, instruction_index, make_dynamic_stack_rejection_record(
                                      context, instruction_index, message));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_dynamic_stack_save(
    const module::BlockLoweringContext& context,
    const prepare::PreparedDynamicStackOp& op,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* result_home =
      find_dynamic_stack_value_home(context.function, op.result_value_name);
  if (result_home == nullptr) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack stack-save lowering requires a prepared result home",
        diagnostics);
  }
  if (const auto reg = dynamic_stack_register_home_name(*result_home); reg.has_value()) {
    return make_dynamic_stack_assembler_instruction(
        context, instruction_index, std::vector<std::string>{"mov " + *reg + ", sp"});
  }
  if (const auto address =
          dynamic_stack_stack_home_address(context.function, *result_home);
      address.has_value()) {
    return make_dynamic_stack_assembler_instruction(
        context,
        instruction_index,
        std::vector<std::string>{"mov x16, sp", "str x16, " + *address});
  }
  if (dynamic_stack_home_requires_stable_frame_pointer(context.function, *result_home)) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack stack-save lowering requires stack-slot homes to use a stable frame-pointer base",
        diagnostics);
  }
  return make_dynamic_stack_failure(
      context,
      instruction_index,
      "AArch64 dynamic-stack stack-save lowering requires a register or stack-slot result home",
      diagnostics);
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_dynamic_alloca(
    const module::BlockLoweringContext& context,
    const prepare::PreparedDynamicStackOp& op,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto element_size = dynamic_stack_element_size_bytes(op);
  if (!element_size.has_value() || *element_size > 65535) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires a printable nonzero element size",
        diagnostics);
  }

  const auto* count_home =
      find_dynamic_stack_value_home(context.function, op.operand_value_name);
  const auto* result_home =
      find_dynamic_stack_value_home(context.function, op.result_value_name);
  if (count_home == nullptr || result_home == nullptr) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires prepared operand and result homes",
        diagnostics);
  }

  std::vector<std::string> lines;
  std::string count_register;
  if (const auto reg = dynamic_stack_register_home_name(*count_home); reg.has_value()) {
    count_register = *reg;
  } else if (const auto address =
                 dynamic_stack_stack_home_address(context.function, *count_home);
             address.has_value()) {
    lines.push_back("ldr x16, " + *address);
    count_register = "x16";
  } else if (dynamic_stack_home_requires_stable_frame_pointer(context.function, *count_home)) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires stack-slot count homes to use a stable frame-pointer base",
        diagnostics);
  } else {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires a register or stack-slot count home",
        diagnostics);
  }

  std::string byte_count_register = count_register;
  if (byte_count_register == "x17") {
    lines.push_back("mov x16, x17");
    byte_count_register = "x16";
  }
  if (*element_size != 1) {
    if (byte_count_register != "x16") {
      lines.push_back("mov x16, " + byte_count_register);
      byte_count_register = "x16";
    }
    lines.push_back("mov x17, #" + std::to_string(*element_size));
    lines.push_back("mul x16, x16, x17");
  }

  lines.push_back("mov x17, sp");
  lines.push_back("sub x17, x17, " + byte_count_register);
  lines.push_back("and x17, x17, #-16");
  lines.push_back("mov sp, x17");

  if (const auto reg = dynamic_stack_register_home_name(*result_home); reg.has_value()) {
    lines.push_back("mov " + *reg + ", sp");
  } else if (const auto address =
                 dynamic_stack_stack_home_address(context.function, *result_home);
             address.has_value()) {
    lines.push_back("str x17, " + *address);
  } else if (dynamic_stack_home_requires_stable_frame_pointer(context.function, *result_home)) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires stack-slot result homes to use a stable frame-pointer base",
        diagnostics);
  } else {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires a register or stack-slot result home",
        diagnostics);
  }

  return make_dynamic_stack_assembler_instruction(
      context, instruction_index, std::move(lines));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_dynamic_stack_restore(
    const module::BlockLoweringContext& context,
    const prepare::PreparedDynamicStackOp& op,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* operand_home =
      find_dynamic_stack_value_home(context.function, op.operand_value_name);
  if (operand_home == nullptr) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack restore lowering requires a prepared operand home",
        diagnostics);
  }
  if (const auto reg = dynamic_stack_register_home_name(*operand_home); reg.has_value()) {
    return make_dynamic_stack_assembler_instruction(
        context, instruction_index, std::vector<std::string>{"mov sp, " + *reg});
  }
  if (const auto address =
          dynamic_stack_stack_home_address(context.function, *operand_home);
      address.has_value()) {
    return make_dynamic_stack_assembler_instruction(
        context,
        instruction_index,
        std::vector<std::string>{"ldr x16, " + *address, "mov sp, x16"});
  }
  if (dynamic_stack_home_requires_stable_frame_pointer(context.function, *operand_home)) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack restore lowering requires stack-slot operand homes to use a stable frame-pointer base",
        diagnostics);
  }
  return make_dynamic_stack_failure(
      context,
      instruction_index,
      "AArch64 dynamic-stack restore lowering requires a register or stack-slot operand home",
      diagnostics);
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_dynamic_stack_helper_call(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto kind = dynamic_stack_helper_kind(call_inst.callee);
  if (!kind.has_value()) {
    return std::nullopt;
  }

  const auto* op = find_dynamic_stack_op(context, instruction_index, *kind);
  if (op == nullptr) {
    constexpr std::string_view message =
        "AArch64 dynamic-stack helper lowering requires prepared dynamic-stack operation authority";
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        std::string{message});
    return make_bir_machine_instruction(
        context, instruction_index, make_dynamic_stack_rejection_record(
                                        context, instruction_index, message));
  }

  switch (*kind) {
    case prepare::PreparedDynamicStackOpKind::StackSave:
      return lower_dynamic_stack_save(context, *op, instruction_index, diagnostics);
    case prepare::PreparedDynamicStackOpKind::DynamicAlloca:
      return lower_dynamic_alloca(context, *op, instruction_index, diagnostics);
    case prepare::PreparedDynamicStackOpKind::StackRestore:
      return lower_dynamic_stack_restore(context, *op, instruction_index, diagnostics);
  }
  return make_dynamic_stack_failure(
      context,
      instruction_index,
      "AArch64 dynamic-stack helper lowering visited an unsupported operation kind",
      diagnostics);
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (auto dynamic_stack = lower_dynamic_stack_helper_call(
          context, call_inst, instruction_index, diagnostics);
      dynamic_stack.has_value()) {
    return dynamic_stack;
  }

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
  auto record_call_boundary_destination =
      [&](const module::MachineInstruction& instruction) {
    const auto* move_record =
        std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
    if (move_record != nullptr && move_record->destination_register.has_value()) {
      record_emitted_scalar_register(scalar_state,
                                     move_record->destination_register->value_name,
                                     *move_record->destination_register);
    }
  };
  auto retarget_call_boundary_source_to_emitted_scalar =
      [&](module::MachineInstruction& instruction) {
    auto* move_record =
        std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
    if (move_record == nullptr || !move_record->source_memory.has_value() ||
        !move_record->source_memory->result_value_name.has_value()) {
      return;
    }
    const auto emitted = find_emitted_scalar_register(
        scalar_state,
        *move_record->source_memory->result_value_name);
    if (!emitted.has_value()) {
      return;
    }
    move_record->source_register = *emitted;
    move_record->source_memory.reset();
  };
  for (auto& block_entry_move : lower_value_moves(
           context,
           prepare::PreparedMovePhase::BlockEntry,
           0,
           diagnostics)) {
    record_call_boundary_destination(block_entry_move);
    block.instructions.push_back(std::move(block_entry_move));
  }
  if (context.bir_block != nullptr) {
    for (std::size_t instruction_index = 0;
         instruction_index < context.bir_block->insts.size();
         ++instruction_index) {
      const auto& inst = context.bir_block->insts[instruction_index];
      if (const auto* call = std::get_if<bir::CallInst>(&inst)) {
        if (auto dynamic_stack = lower_dynamic_stack_helper_call(
                context, *call, instruction_index, diagnostics)) {
          block.instructions.push_back(std::move(*dynamic_stack));
          ++result.visited_operations;
          continue;
        }
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
          auto materialized_addresses =
              lower_address_materializations(context, instruction_index, diagnostics);
          for (auto& materialized : materialized_addresses) {
            if (const auto* address_record =
                    std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
                address_record != nullptr && address_record->result_register.has_value()) {
              record_emitted_scalar_register(scalar_state,
                                             address_record->result_value_name,
                                             *address_record->result_register);
            }
            block.instructions.push_back(std::move(materialized));
          }
          auto before_call_moves =
              lower_before_call_moves(context, *call_plan, instruction_index, diagnostics);
          for (auto& before_call_move : before_call_moves) {
            retarget_call_boundary_source_to_emitted_scalar(before_call_move);
            block.instructions.push_back(std::move(before_call_move));
          }
        }
        if (auto lowered = lower_call_instruction(
                context, *call, instruction_index, diagnostics)) {
          block.instructions.push_back(std::move(*lowered));
          clear_call_clobbered_emitted_scalar_registers(scalar_state);
          if (call_plan != nullptr) {
            auto after_call_moves =
                lower_after_call_moves(context, *call_plan, instruction_index, diagnostics);
            for (auto& after_call_move : after_call_moves) {
              record_call_boundary_destination(after_call_move);
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
      } else if (auto lowered = lower_scalar_control_value_instruction(
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
    for (auto& edge_source :
         lower_predecessor_select_parallel_copy_sources(context, scalar_state, diagnostics)) {
      block.instructions.push_back(std::move(edge_source));
    }
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
