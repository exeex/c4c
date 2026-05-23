#include "dispatch.hpp"

#include "instruction.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;

void dynamic_stack_append_call_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
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
  dynamic_stack_append_call_diagnostic(diagnostics,
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


}  // namespace

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
    dynamic_stack_append_call_diagnostic(
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


}  // namespace c4c::backend::aarch64::codegen
