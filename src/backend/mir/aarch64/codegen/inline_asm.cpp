#include "inline_asm.hpp"

#include "../abi/abi.hpp"
#include "memory.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace prepare = c4c::backend::prepare;

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

[[nodiscard]] const prepare::PreparedInlineAsmCarrier* find_prepared_inline_asm_carrier(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr || context.function.control_flow == nullptr) {
    return nullptr;
  }
  const auto* function_carriers = prepare::find_prepared_inline_asm_carriers(
      *context.function.prepared, context.function.control_flow->function_name);
  if (function_carriers == nullptr) {
    return nullptr;
  }
  for (const auto& carrier : function_carriers->carriers) {
    if (carrier.block_index == context.block_index &&
        carrier.inst_index == instruction_index) {
      return &carrier;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<std::string> inline_asm_operand_name(
    const bir::CallInst& call_inst,
    std::size_t constraint_index) {
  if (!call_inst.inline_asm.has_value()) {
    return std::nullopt;
  }
  for (const auto& operand : call_inst.inline_asm->operands) {
    if (operand.constraint_index == constraint_index && operand.name.has_value()) {
      return operand.name;
    }
  }
  return std::nullopt;
}

[[nodiscard]] MachineEffectResource inline_asm_effect_from_operand(
    const OperandRecord& operand) {
  MachineEffectResource effect;
  effect.operand = operand;
  if (const auto* reg = std::get_if<RegisterOperand>(&operand.payload);
      operand.kind == OperandKind::Register && reg != nullptr) {
    effect.kind = MachineEffectResourceKind::Register;
    effect.value_id = reg->value_id;
    effect.value_name = reg->value_name;
    effect.reg = reg->reg;
    return effect;
  }
  if (const auto* immediate = std::get_if<ImmediateOperand>(&operand.payload);
      operand.kind == OperandKind::Immediate && immediate != nullptr) {
    effect.kind = MachineEffectResourceKind::PreparedValue;
    effect.value_id = immediate->source_value_id;
    effect.value_name = immediate->source_value_name;
    return effect;
  }
  return effect;
}

[[nodiscard]] bool inline_asm_clobber_has_malformed_spelling(std::string_view clobber) {
  if (clobber.empty()) {
    return true;
  }
  for (const char ch : clobber) {
    if (ch == '~' || ch == '{' || ch == '}' || ch == ',' || ch == ' ' ||
        ch == '\t' || ch == '\n' || ch == '\r') {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::optional<unsigned> inline_asm_gpr_clobber_index(
    std::string_view clobber) {
  if (clobber.size() < 2 || (clobber.front() != 'x' && clobber.front() != 'w')) {
    return std::nullopt;
  }
  unsigned value = 0;
  for (std::size_t index = 1; index < clobber.size(); ++index) {
    const char ch = clobber[index];
    if (ch < '0' || ch > '9') {
      return std::nullopt;
    }
    value = value * 10 + static_cast<unsigned>(ch - '0');
    if (value > 30) {
      return std::nullopt;
    }
  }
  return value;
}

[[nodiscard]] std::optional<MachineEffectResource> inline_asm_clobber_effect(
    std::string_view clobber,
    std::string* diagnostic_fact) {
  if (inline_asm_clobber_has_malformed_spelling(clobber)) {
    *diagnostic_fact = "malformed_inline_asm_clobber:";
    diagnostic_fact->append(clobber);
    return std::nullopt;
  }
  if (clobber == "memory") {
    return MachineEffectResource{.kind = MachineEffectResourceKind::Memory};
  }
  if (clobber == "cc") {
    return MachineEffectResource{.kind = MachineEffectResourceKind::Flags};
  }
  if (clobber == "sp" || clobber == "wsp" || clobber == "xzr" ||
      clobber == "wzr") {
    *diagnostic_fact = "target_invalid_inline_asm_clobber:";
    diagnostic_fact->append(clobber);
    return std::nullopt;
  }
  const auto index = inline_asm_gpr_clobber_index(clobber);
  if (!index.has_value()) {
    *diagnostic_fact = "unknown_inline_asm_clobber:";
    diagnostic_fact->append(clobber);
    return std::nullopt;
  }

  const std::string canonical_name = "x" + std::to_string(*index);
  const auto converted = abi::convert_prepared_register(
      canonical_name,
      prepare::PreparedRegisterBank::Gpr,
      prepare::PreparedRegisterClass::General,
      abi::RegisterView::X);
  if (!converted.reg.has_value()) {
    *diagnostic_fact = "target_invalid_inline_asm_clobber:";
    diagnostic_fact->append(clobber);
    return std::nullopt;
  }

  const OperandRecord operand = make_register_operand(RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::CallAbi,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = abi::RegisterView::X,
      .contiguous_width = 1,
      .occupied_register_references = {*converted.reg},
      .occupied_registers = {canonical_name},
  });
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::Register,
      .operand = operand,
      .reg = *converted.reg,
  };
}

[[nodiscard]] std::optional<std::vector<MachineEffectResource>>
inline_asm_clobber_effects(
    const std::vector<std::string>& clobbers,
    std::string* diagnostic_fact) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(clobbers.size());
  for (const auto& clobber : clobbers) {
    auto effect = inline_asm_clobber_effect(clobber, diagnostic_fact);
    if (!effect.has_value()) {
      return std::nullopt;
    }
    effects.push_back(std::move(*effect));
  }
  return effects;
}

[[nodiscard]] std::optional<RegisterOperand> make_inline_asm_register_operand(
    const prepare::PreparedValueHome& home,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value()) {
    return std::nullopt;
  }

  const auto converted =
      abi::convert_prepared_register(*home.register_name,
                                     prepare::PreparedRegisterBank::Gpr,
                                     prepare::PreparedRegisterClass::General,
                                     std::nullopt);
  if (!converted.reg.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::RegisterConversionFailed,
        context,
        instruction_index,
        converted.error.has_value()
            ? converted.error->message
            : "AArch64 inline-asm register home could not be converted");
    return std::nullopt;
  }

  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::ValueHome,
      .value_id = home.value_id,
      .value_name = home.value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .contiguous_width = 1,
      .occupied_register_references = {*converted.reg},
      .occupied_registers = {abi::register_name(*converted.reg)},
  };
}

[[nodiscard]] std::string inline_asm_carrier_error_message(
    std::string_view fact,
    const prepare::PreparedInlineAsmCarrier* carrier = nullptr) {
  std::string message =
      "AArch64 inline-asm lowering requires a complete prepared inline-asm carrier";
  if (!fact.empty()) {
    message += "; missing fact=";
    message += fact;
  } else if (carrier != nullptr && !carrier->missing_required_facts.empty()) {
    message += "; missing fact=";
    message += carrier->missing_required_facts.front();
  }
  return message;
}

[[nodiscard]] std::optional<OperandRecord> make_inline_asm_memory_address_operand(
    const prepare::PreparedInlineAsmOperand& operand,
    const bir::MemoryAddress& address,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::string_view unsupported_fact) {
  if (!operand.value.has_value() ||
      address.base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
      address.base_value != *operand.value || !operand.home.has_value() ||
      operand.home->kind != prepare::PreparedValueHomeKind::Register ||
      !operand.home->register_name.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        inline_asm_carrier_error_message(unsupported_fact));
    return std::nullopt;
  }

  const auto reg = make_inline_asm_register_operand(
      *operand.home, diagnostics, context, instruction_index);
  if (!reg.has_value()) {
    return std::nullopt;
  }

  return make_memory_operand(MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .base_kind = MemoryBaseKind::PointerValue,
      .base_register = *reg,
      .pointer_value_name = operand.value_name,
      .pointer_value_id = operand.home->value_id,
      .byte_offset = address.byte_offset,
      .size_bytes = address.size_bytes,
      .align_bytes = address.align_bytes,
      .address_space = address.address_space,
      .is_volatile = address.is_volatile,
      .can_use_base_plus_offset = true,
  });
}

[[nodiscard]] const prepare::PreparedInlineAsmOperand*
find_inline_asm_prepared_output_operand(
    const prepare::PreparedInlineAsmCarrier& carrier,
    std::size_t output_index) {
  for (const auto& operand : carrier.operands) {
    if (operand.kind == bir::InlineAsmOperandKind::RegisterOutput &&
        operand.output_index.has_value() && *operand.output_index == output_index) {
      return &operand;
    }
  }
  return nullptr;
}

[[nodiscard]] bool inline_asm_prepared_home_is_concrete_register(
    const prepare::PreparedValueHome& home) {
  return home.kind == prepare::PreparedValueHomeKind::Register &&
         home.register_name.has_value();
}

[[nodiscard]] bool inline_asm_identity_matches_register_constraint(
    const prepare::PreparedTargetRegisterIdentity& identity) {
  return identity.bank == prepare::PreparedRegisterBank::Gpr &&
         identity.register_class == prepare::PreparedRegisterClass::General;
}

[[nodiscard]] bool require_inline_asm_tied_home_agreement(
    const prepare::PreparedInlineAsmCarrier& carrier,
    const prepare::PreparedInlineAsmOperand& operand,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  const auto append_tied_diagnostic = [&](std::string_view fact) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        inline_asm_carrier_error_message(fact));
  };

  if (!operand.tied_output_index.has_value()) {
    append_tied_diagnostic("missing_tied_output_index");
    return false;
  }
  if (find_inline_asm_prepared_output_operand(carrier, *operand.tied_output_index) ==
      nullptr) {
    append_tied_diagnostic("missing_tied_output_operand");
    return false;
  }
  if (!operand.home.has_value()) {
    append_tied_diagnostic("missing_tied_input_register_home");
    return false;
  }
  if (!carrier.result_home.has_value()) {
    append_tied_diagnostic("missing_tied_output_register_home");
    return false;
  }
  if (!inline_asm_prepared_home_is_concrete_register(*operand.home) ||
      !inline_asm_prepared_home_is_concrete_register(*carrier.result_home)) {
    append_tied_diagnostic("tied_input_output_home_requires_concrete_registers");
    return false;
  }
  const auto tied_identity = operand.home->target_register_identity;
  if (!tied_identity.has_value()) {
    append_tied_diagnostic("target_invalid_tied_input_register_home");
    return false;
  }
  const auto output_identity = carrier.result_home->target_register_identity;
  if (!output_identity.has_value()) {
    append_tied_diagnostic("target_invalid_tied_output_register_home");
    return false;
  }
  if (!inline_asm_identity_matches_register_constraint(*tied_identity) ||
      !inline_asm_identity_matches_register_constraint(*output_identity)) {
    append_tied_diagnostic("tied_input_output_home_incompatible_register_class");
    return false;
  }
  if (*tied_identity != *output_identity) {
    append_tied_diagnostic("tied_input_output_home_mismatch");
    return false;
  }
  if (!operand.tied_home_authority.has_value()) {
    append_tied_diagnostic("missing_tied_home_coallocation_authority");
    return false;
  }
  if (operand.tied_home_authority->tied_output_index != *operand.tied_output_index) {
    append_tied_diagnostic("tied_home_coallocation_authority_output_mismatch");
    return false;
  }
  if (operand.tied_home_authority->shared_register != *tied_identity) {
    append_tied_diagnostic("tied_home_coallocation_authority_home_mismatch");
    return false;
  }
  return true;
}

[[nodiscard]] module::MachineInstruction make_inline_asm_machine_instruction(
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

[[nodiscard]] bool decimal_digits_only(std::string_view text) {
  if (text.empty()) {
    return false;
  }
  for (const char ch : text) {
    if (ch < '0' || ch > '9') {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::optional<std::size_t> parse_decimal_index(std::string_view text) {
  if (!decimal_digits_only(text)) {
    return std::nullopt;
  }
  std::size_t value = 0;
  for (const char ch : text) {
    const auto digit = static_cast<std::size_t>(ch - '0');
    if (value > (static_cast<std::size_t>(-1) - digit) / 10U) {
      return std::nullopt;
    }
    value = value * 10U + digit;
  }
  return value;
}

[[nodiscard]] std::string register_name(const RegisterOperand& operand) {
  return abi::register_name(operand.reg);
}

[[nodiscard]] std::optional<std::string> register_name_with_view(
    const RegisterOperand& operand,
    abi::RegisterView view) {
  if (!abi::is_gp_register(operand.reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::gp_register(operand.reg.index, view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*viewed);
}

[[nodiscard]] const InlineAsmMachineOperandRecord* find_inline_asm_constraint_operand(
    const AssemblerInstructionRecord& assembler,
    std::size_t constraint_index) {
  for (const auto& operand : assembler.inline_asm_operands) {
    if (operand.constraint_index == constraint_index) {
      return &operand;
    }
  }
  return nullptr;
}

[[nodiscard]] const InlineAsmMachineOperandRecord* find_inline_asm_output_operand(
    const AssemblerInstructionRecord& assembler,
    std::size_t output_index) {
  for (const auto& operand : assembler.inline_asm_operands) {
    if (operand.kind == bir::InlineAsmOperandKind::RegisterOutput &&
        operand.output_index.has_value() && *operand.output_index == output_index) {
      return &operand;
    }
  }
  return nullptr;
}

[[nodiscard]] bool inline_asm_home_is_concrete_register(
    const prepare::PreparedValueHome& home) {
  return home.kind == prepare::PreparedValueHomeKind::Register &&
         home.register_name.has_value();
}

[[nodiscard]] std::optional<prepare::PreparedTargetRegisterIdentity>
inline_asm_selected_register_identity(const OperandRecord& selected) {
  if (selected.kind != OperandKind::Register) {
    return std::nullopt;
  }
  const auto* reg = std::get_if<RegisterOperand>(&selected.payload);
  if (reg == nullptr) {
    return std::nullopt;
  }
  if (reg->reg.bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  return prepare::PreparedTargetRegisterIdentity{
      .target_arch = c4c::TargetArch::Aarch64,
      .bank = prepare::PreparedRegisterBank::Gpr,
      .register_class = prepare::PreparedRegisterClass::General,
      .physical_index = reg->reg.index,
  };
}

struct InlineAsmNamedOperandLookup {
  const InlineAsmMachineOperandRecord* operand = nullptr;
  std::string diagnostic;
};

[[nodiscard]] InlineAsmNamedOperandLookup find_inline_asm_named_operand(
    const AssemblerInstructionRecord& assembler,
    std::string_view name) {
  if (name.empty()) {
    return {.diagnostic = "inline-asm template has missing named operand name"};
  }

  const InlineAsmMachineOperandRecord* found = nullptr;
  std::size_t matching_names = 0;
  for (const auto& operand : assembler.inline_asm_operands) {
    if (!operand.name.has_value() || operand.name->empty()) {
      continue;
    }
    if (std::string_view{*operand.name} == name) {
      found = &operand;
      ++matching_names;
    }
  }

  if (matching_names > 1) {
    return {.diagnostic = "inline-asm template references duplicate named operand"};
  }
  if (found == nullptr) {
    return {.diagnostic = "inline-asm template references unknown named operand"};
  }
  return {.operand = found};
}

[[nodiscard]] bool inline_asm_constraint_matches_kind(
    const InlineAsmMachineOperandRecord& operand) {
  switch (operand.kind) {
    case bir::InlineAsmOperandKind::RegisterInput:
      return operand.constraint == "r";
    case bir::InlineAsmOperandKind::RegisterOutput:
      return operand.constraint == "=r";
    case bir::InlineAsmOperandKind::TiedInput:
      return decimal_digits_only(operand.constraint);
    case bir::InlineAsmOperandKind::IntegerImmediateInput:
      return operand.constraint == "i" || operand.constraint == "I";
    case bir::InlineAsmOperandKind::MemoryInput:
      return operand.constraint == "m";
    case bir::InlineAsmOperandKind::AddressInput:
      return operand.constraint == "p";
    case bir::InlineAsmOperandKind::Clobber:
    case bir::InlineAsmOperandKind::Unsupported:
      return false;
  }
  return false;
}

[[nodiscard]] std::optional<std::string> inline_asm_register_operand_text(
    const RegisterOperand& reg,
    std::optional<char> modifier) {
  if (!modifier.has_value()) {
    return register_name(reg);
  }
  switch (*modifier) {
    case 'w':
      return register_name_with_view(reg, abi::RegisterView::W);
    case 'x':
      return register_name_with_view(reg, abi::RegisterView::X);
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<std::string> inline_asm_operand_text(
    const AssemblerInstructionRecord& assembler,
    const InlineAsmMachineOperandRecord& operand,
    std::optional<char> modifier,
    std::string* diagnostic) {
  if (operand.kind == bir::InlineAsmOperandKind::Clobber) {
    *diagnostic = "inline-asm clobber operand requires structured clobber authority";
    return std::nullopt;
  }
  if (operand.kind == bir::InlineAsmOperandKind::Unsupported) {
    *diagnostic = "inline-asm operand kind is unsupported for selected printer";
    return std::nullopt;
  }
  if (!inline_asm_constraint_matches_kind(operand)) {
    *diagnostic = "inline-asm operand has unsupported constraint for selected printer";
    return std::nullopt;
  }

  const InlineAsmMachineOperandRecord* printable_operand = &operand;
  if (operand.kind == bir::InlineAsmOperandKind::TiedInput) {
    if (!operand.tied_output_index.has_value()) {
      *diagnostic = "inline-asm tied input is missing tied output index";
      return std::nullopt;
    }
    printable_operand = find_inline_asm_output_operand(assembler, *operand.tied_output_index);
    if (printable_operand == nullptr || !printable_operand->selected_operand.has_value()) {
      *diagnostic = "inline-asm tied input is missing selected tied output operand";
      return std::nullopt;
    }
    if (!operand.selected_operand.has_value()) {
      *diagnostic = "inline-asm tied input is missing selected operand";
      return std::nullopt;
    }
    if (!operand.home.has_value() || !printable_operand->home.has_value()) {
      *diagnostic = "inline-asm tied input is missing prepared tied home";
      return std::nullopt;
    }
    if (!inline_asm_home_is_concrete_register(*operand.home) ||
        !inline_asm_home_is_concrete_register(*printable_operand->home)) {
      *diagnostic = "inline-asm tied input requires concrete prepared register homes";
      return std::nullopt;
    }
    if (!operand.home->target_register_identity.has_value() ||
        !printable_operand->home->target_register_identity.has_value()) {
      *diagnostic =
          "inline-asm tied input is missing prepared coallocation authority";
      return std::nullopt;
    }
    if (!inline_asm_identity_matches_register_constraint(
            *operand.home->target_register_identity) ||
        !inline_asm_identity_matches_register_constraint(
            *printable_operand->home->target_register_identity)) {
      *diagnostic =
          "inline-asm tied input has incompatible prepared register class";
      return std::nullopt;
    }
    if (*operand.home->target_register_identity !=
        *printable_operand->home->target_register_identity) {
      *diagnostic = "inline-asm tied input prepared home disagrees with output";
      return std::nullopt;
    }
    const auto tied_selected_identity =
        inline_asm_selected_register_identity(*operand.selected_operand);
    if (!tied_selected_identity.has_value() ||
        *tied_selected_identity != *operand.home->target_register_identity) {
      *diagnostic =
          "inline-asm tied input selected register disagrees with prepared home";
      return std::nullopt;
    }
    const auto output_selected_identity =
        inline_asm_selected_register_identity(*printable_operand->selected_operand);
    if (!output_selected_identity.has_value() ||
        *output_selected_identity != *printable_operand->home->target_register_identity) {
      *diagnostic =
          "inline-asm tied output selected register disagrees with prepared home";
      return std::nullopt;
    }
  }

  if (!printable_operand->selected_operand.has_value()) {
    *diagnostic = "inline-asm operand is missing selected operand";
    return std::nullopt;
  }
  const auto& selected = *printable_operand->selected_operand;
  if (const auto* reg = std::get_if<RegisterOperand>(&selected.payload);
      selected.kind == OperandKind::Register && reg != nullptr) {
    const auto text = inline_asm_register_operand_text(*reg, modifier);
    if (!text.has_value()) {
      *diagnostic = modifier.has_value()
                        ? "inline-asm register operand has unsupported template modifier"
                        : "inline-asm register operand is not printable";
      return std::nullopt;
    }
    return text;
  }
  if (const auto* immediate = std::get_if<ImmediateOperand>(&selected.payload);
      selected.kind == OperandKind::Immediate && immediate != nullptr) {
    if (modifier.has_value()) {
      *diagnostic = "inline-asm immediate operand has unsupported template modifier";
      return std::nullopt;
    }
    if (immediate->kind != ImmediateKind::SignedInteger) {
      *diagnostic = "inline-asm immediate operand is not a signed integer";
      return std::nullopt;
    }
    return std::to_string(immediate->signed_value);
  }
  if (const auto* memory = std::get_if<MemoryOperand>(&selected.payload);
      selected.kind == OperandKind::Memory && memory != nullptr) {
    if (modifier.has_value()) {
      *diagnostic = "inline-asm memory operand has unsupported template modifier";
      return std::nullopt;
    }
    if (memory->support != MemoryOperandSupportKind::Prepared ||
        !memory->can_use_base_plus_offset) {
      *diagnostic = "inline-asm memory operand is not a prepared base+offset address";
      return std::nullopt;
    }
    const auto text = memory_address(*memory);
    if (text.empty()) {
      *diagnostic = "inline-asm memory operand is not printable";
      return std::nullopt;
    }
    return text;
  }

  *diagnostic = "inline-asm operand kind is outside the selected printer subset";
  return std::nullopt;
}

void append_inline_asm_output_character(std::vector<std::string>* lines,
                                        char ch) {
  if (ch == '\n') {
    lines->emplace_back();
    return;
  }
  if (ch != '\r') {
    lines->back().push_back(ch);
  }
}

void append_inline_asm_output_text(std::vector<std::string>* lines,
                                   std::string_view text) {
  for (const char ch : text) {
    append_inline_asm_output_character(lines, ch);
  }
}

}  // namespace

bool has_prepared_inline_asm_carrier(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  return find_prepared_inline_asm_carrier(context, instruction_index) != nullptr;
}

std::optional<module::MachineInstruction> lower_inline_asm_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (!call_inst.inline_asm.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        inline_asm_carrier_error_message("missing_retained_inline_asm_metadata"));
    return std::nullopt;
  }
  if (context.function.target_profile == nullptr ||
      !abi::is_aarch64_target(*context.function.target_profile)) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        inline_asm_carrier_error_message("target_profile_is_not_aarch64"));
    return std::nullopt;
  }

  const auto* carrier = find_prepared_inline_asm_carrier(context, instruction_index);
  if (carrier == nullptr) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        inline_asm_carrier_error_message("missing_prepared_inline_asm_carrier"));
    return std::nullopt;
  }
  if (carrier->carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      !carrier->missing_required_facts.empty()) {
    append_call_diagnostic(diagnostics,
                           module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
                           context,
                           instruction_index,
                           inline_asm_carrier_error_message({}, carrier));
    return std::nullopt;
  }
  std::string clobber_diagnostic_fact;
  auto clobber_effects =
      inline_asm_clobber_effects(carrier->clobbers, &clobber_diagnostic_fact);
  if (!clobber_effects.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        inline_asm_carrier_error_message(clobber_diagnostic_fact));
    return std::nullopt;
  }
  AssemblerInstructionRecord record{
      .has_inline_asm_payload = true,
      .side_effects = carrier->side_effects,
      .inline_asm_template = carrier->asm_text,
      .inline_asm_constraints = carrier->constraints,
      .inline_asm_has_named_operand_references =
          carrier->has_named_operand_references,
      .inline_asm_has_template_modifiers = carrier->has_template_modifiers,
      .inline_asm_clobbers = carrier->clobbers,
      .inline_asm_result = carrier->result,
      .inline_asm_result_value_name = carrier->result_value_name,
      .inline_asm_result_home = carrier->result_home,
  };

  record.inline_asm_operands.reserve(carrier->operands.size());
  record.operands.reserve(carrier->operands.size());
  for (const auto& operand : carrier->operands) {
    InlineAsmMachineOperandRecord selected{
        .kind = operand.kind,
        .constraint_index = operand.constraint_index,
        .constraint = operand.constraint,
        .arg_index = operand.arg_index,
        .output_index = operand.output_index,
        .tied_output_index = operand.tied_output_index,
        .name = inline_asm_operand_name(call_inst, operand.constraint_index),
        .value = operand.value,
        .value_name = operand.value_name,
        .home = operand.home,
        .immediate_value = operand.immediate_value,
    };

    switch (operand.kind) {
      case bir::InlineAsmOperandKind::RegisterOutput: {
        if (!carrier->result_home.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message("missing_result_register_home"));
          return std::nullopt;
        }
        if (carrier->result_home->kind != prepare::PreparedValueHomeKind::Register ||
            !carrier->result_home->register_name.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message("result_home_not_a_gpr"));
          return std::nullopt;
        }
        selected.value = carrier->result;
        selected.value_name = carrier->result_value_name;
        selected.home = carrier->result_home;
        const auto reg = make_inline_asm_register_operand(
            *carrier->result_home, diagnostics, context, instruction_index);
        if (!reg.has_value()) {
          return std::nullopt;
        }
        selected.selected_operand = make_register_operand(*reg);
        break;
      }
      case bir::InlineAsmOperandKind::RegisterInput: {
        if (!operand.home.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message("missing_register_operand_home"));
          return std::nullopt;
        }
        if (operand.home->kind != prepare::PreparedValueHomeKind::Register ||
            !operand.home->register_name.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message("register_operand_home_not_a_gpr"));
          return std::nullopt;
        }
        const auto reg = make_inline_asm_register_operand(
            *operand.home, diagnostics, context, instruction_index);
        if (!reg.has_value()) {
          return std::nullopt;
        }
        selected.selected_operand = make_register_operand(*reg);
        break;
      }
      case bir::InlineAsmOperandKind::TiedInput: {
        if (!require_inline_asm_tied_home_agreement(
                *carrier, operand, diagnostics, context, instruction_index)) {
          return std::nullopt;
        }
        const auto reg = make_inline_asm_register_operand(
            *operand.home, diagnostics, context, instruction_index);
        if (!reg.has_value()) {
          return std::nullopt;
        }
        selected.selected_operand = make_register_operand(*reg);
        break;
      }
      case bir::InlineAsmOperandKind::IntegerImmediateInput: {
        if (!operand.immediate_value.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message("missing_integer_immediate_value"));
          return std::nullopt;
        }
        selected.selected_operand = make_immediate_operand(ImmediateOperand{
            .kind = ImmediateKind::SignedInteger,
            .type = operand.value.has_value() ? operand.value->type : bir::TypeKind::Void,
            .signed_value = *operand.immediate_value,
            .unsigned_value = static_cast<std::uint64_t>(*operand.immediate_value),
            .source_value_id = operand.home.has_value()
                                   ? std::optional<prepare::PreparedValueId>{
                                         operand.home->value_id}
                                   : std::nullopt,
            .source_value_name = operand.value_name.value_or(c4c::kInvalidValueName),
        });
        break;
      }
      case bir::InlineAsmOperandKind::MemoryInput:
        if (!operand.memory_address.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message(
                  "unsupported_inline_asm_memory_address_selection"));
          return std::nullopt;
        }
        selected.selected_operand = make_inline_asm_memory_address_operand(
            operand,
            *operand.memory_address,
            diagnostics,
            context,
            instruction_index,
            "unsupported_inline_asm_memory_address_selection");
        if (!selected.selected_operand.has_value()) {
          return std::nullopt;
        }
        break;
      case bir::InlineAsmOperandKind::AddressInput:
        if (!operand.address.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message(
                  "unsupported_inline_asm_address_selection"));
          return std::nullopt;
        }
        selected.selected_operand = make_inline_asm_memory_address_operand(
            operand,
            *operand.address,
            diagnostics,
            context,
            instruction_index,
            "unsupported_inline_asm_address_selection");
        if (!selected.selected_operand.has_value()) {
          return std::nullopt;
        }
        break;
      case bir::InlineAsmOperandKind::Clobber:
        append_call_diagnostic(
            diagnostics,
            module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
            context,
            instruction_index,
            inline_asm_carrier_error_message("unsupported_inline_asm_clobber_operand_kind"));
        return std::nullopt;
      case bir::InlineAsmOperandKind::Unsupported:
        append_call_diagnostic(
            diagnostics,
            module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
            context,
            instruction_index,
            inline_asm_carrier_error_message("unsupported_inline_asm_operand_kind"));
        return std::nullopt;
    }

    if (selected.selected_operand.has_value()) {
      record.operands.push_back(*selected.selected_operand);
    }
    record.inline_asm_operands.push_back(std::move(selected));
  }

  InstructionRecord target = make_assembler_instruction(record);
  target.surface = RecordSurfaceKind::MachineInstructionNode;
  target.selection = MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  target.function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName;
  target.block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  target.operands = record.operands;
  target.defs.clear();
  target.uses.clear();
  target.clobbers = std::move(*clobber_effects);
  for (const auto& inline_operand : record.inline_asm_operands) {
    if (!inline_operand.selected_operand.has_value()) {
      continue;
    }
    const auto effect = inline_asm_effect_from_operand(*inline_operand.selected_operand);
    if (inline_operand.kind == bir::InlineAsmOperandKind::RegisterOutput) {
      target.defs.push_back(effect);
    } else {
      target.uses.push_back(effect);
    }
  }

  return make_inline_asm_machine_instruction(context, instruction_index, std::move(target));
}

InlineAsmSubstitutionResult substitute_inline_asm_template(
    const AssemblerInstructionRecord& assembler) {
  if (!assembler.has_inline_asm_payload) {
    return {.diagnostic = "assembler node is missing inline-asm payload"};
  }
  if (assembler.inline_asm_template.empty()) {
    return {.diagnostic = "inline-asm template is empty"};
  }
  for (const auto& operand : assembler.inline_asm_operands) {
    if (operand.kind == bir::InlineAsmOperandKind::Clobber) {
      return {.diagnostic = "inline-asm clobber operand requires structured clobber authority"};
    }
    if (operand.kind == bir::InlineAsmOperandKind::Unsupported) {
      return {.diagnostic = "inline-asm operand kind is unsupported for selected printer"};
    }
    if (operand.kind == bir::InlineAsmOperandKind::MemoryInput &&
        !operand.selected_operand.has_value()) {
      return {
          .diagnostic =
              "inline-asm memory operand requires selected structured memory address authority"};
    }
    if (operand.kind == bir::InlineAsmOperandKind::AddressInput &&
        !operand.selected_operand.has_value()) {
      return {
          .diagnostic =
              "inline-asm address operand requires selected structured address authority"};
    }
    if (!inline_asm_constraint_matches_kind(operand)) {
      return {.diagnostic = "inline-asm operand has unsupported constraint for selected printer"};
    }
  }

  std::vector<std::string> lines;
  lines.emplace_back();
  const auto& templ = assembler.inline_asm_template;
  for (std::size_t index = 0; index < templ.size(); ++index) {
    const char ch = templ[index];
    if (ch != '%') {
      append_inline_asm_output_character(&lines, ch);
      continue;
    }
    if (index + 1 >= templ.size()) {
      return {.diagnostic = "inline-asm template has unterminated placeholder"};
    }
    if (templ[index + 1] == '%') {
      append_inline_asm_output_character(&lines, '%');
      ++index;
      continue;
    }
    std::optional<char> modifier;
    std::size_t operand_start = index + 1;
    if ((templ[operand_start] >= 'A' && templ[operand_start] <= 'Z') ||
        (templ[operand_start] >= 'a' && templ[operand_start] <= 'z')) {
      modifier = templ[operand_start];
      ++operand_start;
    }
    if (operand_start < templ.size() && templ[operand_start] == '[') {
      const std::size_t name_start = operand_start + 1;
      std::size_t name_end = name_start;
      while (name_end < templ.size() && templ[name_end] != ']') {
        ++name_end;
      }
      if (name_end >= templ.size()) {
        return {.diagnostic = "inline-asm template has malformed named operand reference"};
      }

      const std::string_view name{templ.data() + name_start, name_end - name_start};
      const auto lookup = find_inline_asm_named_operand(assembler, name);
      if (!lookup.diagnostic.empty()) {
        return {.diagnostic = lookup.diagnostic};
      }

      std::string diagnostic;
      const auto replacement =
          inline_asm_operand_text(assembler, *lookup.operand, modifier, &diagnostic);
      if (!replacement.has_value()) {
        return {.diagnostic = std::move(diagnostic)};
      }
      append_inline_asm_output_text(&lines, *replacement);
      index = name_end;
      continue;
    }
    if (operand_start >= templ.size() || templ[operand_start] < '0' ||
        templ[operand_start] > '9') {
      return {.diagnostic = "inline-asm template has malformed placeholder"};
    }
    std::size_t operand_end = operand_start;
    while (operand_end < templ.size() && templ[operand_end] >= '0' &&
           templ[operand_end] <= '9') {
      ++operand_end;
    }
    const auto constraint_index =
        parse_decimal_index(std::string_view{templ}.substr(operand_start,
                                                           operand_end - operand_start));
    if (!constraint_index.has_value()) {
      return {.diagnostic = "inline-asm template operand index is not printable"};
    }
    const auto* operand =
        find_inline_asm_constraint_operand(assembler, *constraint_index);
    if (operand == nullptr) {
      return {.diagnostic = "inline-asm template references unknown operand"};
    }

    std::string diagnostic;
    const auto replacement = inline_asm_operand_text(assembler, *operand, modifier, &diagnostic);
    if (!replacement.has_value()) {
      return {.diagnostic = std::move(diagnostic)};
    }
    append_inline_asm_output_text(&lines, *replacement);
    index = operand_end - 1;
  }

  if (!lines.empty() && lines.back().empty() && templ.back() == '\n') {
    lines.pop_back();
  }
  if (lines.empty()) {
    return {.diagnostic = "inline-asm template produced no printable lines"};
  }
  return {.lines = std::move(lines)};
}

}  // namespace c4c::backend::aarch64::codegen
