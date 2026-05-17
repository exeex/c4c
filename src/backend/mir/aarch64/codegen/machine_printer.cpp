#include "machine_printer.hpp"
#include "alu.hpp"
#include "calls.hpp"
#include "cast_ops.hpp"
#include "comparison.hpp"
#include "f128.hpp"
#include "globals.hpp"
#include "i128_ops.hpp"
#include "memory.hpp"
#include "returns.hpp"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace {

mir::TargetInstructionPrintResult target_unsupported(std::string diagnostic) {
  return mir::target_instruction_unsupported(std::move(diagnostic));
}

mir::TargetInstructionPrintResult target_printed(std::vector<std::string> lines) {
  return mir::target_instruction_lines_printed(std::move(lines));
}

std::string block_label(c4c::FunctionNameId function_name, c4c::BlockLabelId block_label) {
  return ".LBB" + std::to_string(function_name) + "_" + std::to_string(block_label);
}

std::string register_name(const RegisterOperand& operand) {
  return abi::register_name(operand.reg);
}

std::optional<unsigned> integer_type_bit_width(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
      return 1U;
    case bir::TypeKind::I8:
      return 8U;
    case bir::TypeKind::I16:
      return 16U;
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
      return 64U;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<std::string> register_name_with_view(const RegisterOperand& operand,
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

std::optional<std::string> fp_register_name_with_view(const RegisterOperand& operand,
                                                      abi::RegisterView view) {
  if (!abi::is_fp_simd_register(operand.reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::fp_simd_register(operand.reg.index, view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*viewed);
}

bool has_complete_intrinsic_source(const prepare::PreparedIntrinsicCarrier* source) {
  return source != nullptr &&
         source->carrier_kind == prepare::PreparedIntrinsicCarrierKind::Complete;
}

bool has_exact_intrinsic_roles(const std::vector<bir::IntrinsicOperandRole>& roles,
                               std::initializer_list<bir::IntrinsicOperandRole> expected) {
  return roles.size() == expected.size() &&
         std::equal(roles.begin(), roles.end(), expected.begin(), expected.end());
}

bool has_v16i8_unsigned_shape(bir::TypeKind element_type,
                              std::size_t element_width_bytes,
                              std::size_t lane_count,
                              std::size_t total_width_bytes,
                              bir::IntrinsicSignedness signedness) {
  return element_type == bir::TypeKind::I8 && element_width_bytes == 1 &&
         lane_count == 16 && total_width_bytes == 16 &&
         signedness == bir::IntrinsicSignedness::Unsigned;
}

std::optional<abi::RegisterView> integer_register_view(unsigned bit_width) {
  if (bit_width <= 32U) {
    return abi::RegisterView::W;
  }
  if (bit_width == 64U) {
    return abi::RegisterView::X;
  }
  return std::nullopt;
}

std::optional<abi::RegisterView> floating_register_view(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return abi::RegisterView::S;
    case bir::TypeKind::F64:
      return abi::RegisterView::D;
    case bir::TypeKind::Void:
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

bool decimal_digits_only(std::string_view text) {
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

std::optional<std::size_t> parse_decimal_index(std::string_view text) {
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

std::string atomic_loop_label(const AtomicMemoryInstructionRecord& atomic,
                              std::string_view suffix) {
  std::ostringstream out;
  out << ".Latomic_" << atomic.function_name << "_" << atomic.block_label << "_"
      << atomic.instruction_index << "_" << suffix;
  return out.str();
}

std::optional<std::string> atomic_pointer_register_name(
    const AtomicMemoryInstructionRecord& atomic) {
  if (!atomic.pointer_register.has_value()) {
    return std::nullopt;
  }
  return register_name_with_view(*atomic.pointer_register, abi::RegisterView::X);
}

std::optional<std::string> atomic_value_register_name(
    const RegisterOperand& operand,
    std::size_t width_bytes) {
  if (width_bytes == 0 || width_bytes > 8) {
    return std::nullopt;
  }
  return register_name_with_view(operand,
                                 width_bytes == 8 ? abi::RegisterView::X
                                                  : abi::RegisterView::W);
}

std::string_view atomic_plain_load_mnemonic(std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "ldrb";
    case 2:
      return "ldrh";
    case 4:
    case 8:
      return "ldr";
  }
  return {};
}

std::string_view atomic_acquire_load_mnemonic(std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "ldarb";
    case 2:
      return "ldarh";
    case 4:
    case 8:
      return "ldar";
  }
  return {};
}

std::string_view atomic_plain_store_mnemonic(std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "strb";
    case 2:
      return "strh";
    case 4:
    case 8:
      return "str";
  }
  return {};
}

std::string_view atomic_release_store_mnemonic(std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "stlrb";
    case 2:
      return "stlrh";
    case 4:
    case 8:
      return "stlr";
  }
  return {};
}

std::string_view atomic_exclusive_load_mnemonic(std::size_t width_bytes, bool acquire) {
  switch (width_bytes) {
    case 1:
      return acquire ? "ldaxrb" : "ldxrb";
    case 2:
      return acquire ? "ldaxrh" : "ldxrh";
    case 4:
    case 8:
      return acquire ? "ldaxr" : "ldxr";
  }
  return {};
}

std::string_view atomic_exclusive_store_mnemonic(std::size_t width_bytes, bool release) {
  switch (width_bytes) {
    case 1:
      return release ? "stlxrb" : "stxrb";
    case 2:
      return release ? "stlxrh" : "stxrh";
    case 4:
    case 8:
      return release ? "stlxr" : "stxr";
  }
  return {};
}

std::string_view atomic_rmw_operation_mnemonic(bir::AtomicRmwOpcode opcode) {
  switch (opcode) {
    case bir::AtomicRmwOpcode::Exchange:
      return "mov";
    case bir::AtomicRmwOpcode::Add:
      return "add";
    case bir::AtomicRmwOpcode::Sub:
      return "sub";
    case bir::AtomicRmwOpcode::And:
      return "and";
    case bir::AtomicRmwOpcode::Or:
      return "orr";
    case bir::AtomicRmwOpcode::Xor:
      return "eor";
    case bir::AtomicRmwOpcode::None:
      return {};
  }
  return {};
}

bool atomic_ordering_has_acquire(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Acquire ||
         ordering == bir::AtomicOrdering::AcqRel ||
         ordering == bir::AtomicOrdering::SeqCst;
}

struct InlineAsmSubstitutionResult {
  std::optional<std::vector<std::string>> lines;
  std::string diagnostic;
};

const InlineAsmMachineOperandRecord* find_inline_asm_constraint_operand(
    const AssemblerInstructionRecord& assembler,
    std::size_t constraint_index) {
  for (const auto& operand : assembler.inline_asm_operands) {
    if (operand.constraint_index == constraint_index) {
      return &operand;
    }
  }
  return nullptr;
}

const InlineAsmMachineOperandRecord* find_inline_asm_output_operand(
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

bool inline_asm_home_is_concrete_register(
    const prepare::PreparedValueHome& home) {
  return home.kind == prepare::PreparedValueHomeKind::Register &&
         home.register_name.has_value();
}

bool inline_asm_identity_matches_register_constraint(
    const prepare::PreparedTargetRegisterIdentity& identity) {
  return identity.bank == prepare::PreparedRegisterBank::Gpr &&
         identity.register_class == prepare::PreparedRegisterClass::General;
}

std::optional<prepare::PreparedTargetRegisterIdentity>
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

InlineAsmNamedOperandLookup find_inline_asm_named_operand(
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

bool inline_asm_constraint_matches_kind(const InlineAsmMachineOperandRecord& operand) {
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

std::optional<std::string> inline_asm_register_operand_text(
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

std::optional<std::string> inline_asm_operand_text(
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

std::string bad_header(const InstructionRecord& instruction) {
  return std::string("cannot print AArch64 machine node family=") +
         std::string(instruction_family_name(instruction.family)) + " opcode=" +
         std::string(machine_opcode_name(instruction.opcode)) + ": ";
}

std::string_view required_primary_mnemonic(const InstructionRecord& instruction) {
  return machine_instruction_primary_printer_mnemonic(instruction);
}

std::string_view required_auxiliary_mnemonic(const InstructionRecord& instruction) {
  return machine_instruction_auxiliary_printer_mnemonic(instruction);
}

std::optional<std::string> validate_selected_machine_node(const InstructionRecord& instruction) {
  if (instruction.surface != RecordSurfaceKind::MachineInstructionNode) {
    return std::string("printer requires surface machine_instruction_node, got ") +
           std::string(record_surface_kind_name(instruction.surface));
  }
  if (instruction.selection.status != MachineNodeSelectionStatus::Selected) {
    std::string diagnostic = "printer requires selected machine node, got ";
    diagnostic += machine_node_selection_status_name(instruction.selection.status);
    if (!instruction.selection.diagnostic.empty()) {
      diagnostic += ": ";
      diagnostic += instruction.selection.diagnostic;
    }
    return diagnostic;
  }
  return std::nullopt;
}

mir::TargetInstructionPrintResult print_assembler(
    const InstructionRecord& instruction,
    const AssemblerInstructionRecord& assembler) {
  if (instruction.family != InstructionFamily::Assembler) {
    return target_unsupported(bad_header(instruction) +
                              "inline-asm printer requires an assembler machine node");
  }
  const auto substituted = substitute_inline_asm_template(assembler);
  if (!substituted.lines.has_value()) {
    return target_unsupported(bad_header(instruction) + substituted.diagnostic);
  }
  return target_printed(*substituted.lines);
}

mir::TargetInstructionPrintResult print_spill_reload(
    const InstructionRecord& instruction,
    const SpillReloadInstructionRecord& spill_reload) {
  if (!spill_reload.scratch.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload node is missing scratch register");
  }
  if (!spill_reload.stack_offset_bytes.has_value() || !spill_reload.stack_offset_is_prepared_snapshot ||
      !spill_reload.slot_id.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload node is missing prepared stack-slot offset");
  }
  if (spill_reload.slot.support != MemoryOperandSupportKind::Prepared ||
      spill_reload.slot.base_kind != MemoryBaseKind::FrameSlot ||
      !spill_reload.slot.can_use_base_plus_offset) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload node is not a prepared frame-slot address");
  }

  const auto address = memory_address(spill_reload.slot);
  if (address.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload address is not printable");
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "spill/reload pseudo kind is unsupported");
  }

  std::ostringstream out;
  out << mnemonic << " " << register_name(*spill_reload.scratch) << ", " << address;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_branch(const InstructionRecord& instruction,
                                               const BranchInstructionRecord& branch) {
  if (branch.target.function_name == c4c::kInvalidFunctionName ||
      branch.target.block_label == c4c::kInvalidBlockLabel) {
    return target_unsupported(bad_header(instruction) + "branch target identity is missing");
  }
  if (!branch.conditional) {
    const auto mnemonic = comparison_unconditional_branch_mnemonic(instruction);
    if (mnemonic.empty()) {
      return target_unsupported(bad_header(instruction) + "branch mnemonic is not printable");
    }
    std::ostringstream out;
    out << mnemonic << " " << block_label(branch.target.function_name, branch.target.block_label);
    return target_printed({out.str()});
  }
  if (!branch.target_pair.has_value() || !branch.condition.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "conditional branch is missing target pair or condition operand");
  }
  const auto* condition = std::get_if<RegisterOperand>(&branch.condition->payload);
  if (branch.condition->kind != OperandKind::Register || condition == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "materialized-bool branch condition is not a register operand");
  }
  if (branch.condition_record.has_value() &&
      branch.condition_record->form != BranchConditionForm::MaterializedBool) {
    return target_unsupported(bad_header(instruction) +
                              "only materialized-bool conditional branches are printable");
  }

  const auto& targets = *branch.target_pair;
  if (targets.true_target.function_name == c4c::kInvalidFunctionName ||
      targets.true_target.block_label == c4c::kInvalidBlockLabel ||
      targets.false_target.function_name == c4c::kInvalidFunctionName ||
      targets.false_target.block_label == c4c::kInvalidBlockLabel) {
    return target_unsupported(bad_header(instruction) +
                              "conditional branch target identity is missing");
  }

  const auto spelling = comparison_materialized_bool_branch_spelling(instruction);
  if (!spelling.has_value()) {
    return target_unsupported(bad_header(instruction) + "branch mnemonic is not printable");
  }

  std::ostringstream condition_line;
  condition_line << spelling->condition_mnemonic << " " << register_name(*condition) << ", "
                 << block_label(targets.true_target.function_name,
                                targets.true_target.block_label);
  std::ostringstream branch_line;
  branch_line << spelling->branch_mnemonic << " "
              << block_label(targets.false_target.function_name,
                             targets.false_target.block_label);
  return target_printed({condition_line.str(), branch_line.str()});
}

mir::TargetInstructionPrintResult print_memory(const InstructionRecord& instruction,
                                               const MemoryInstructionRecord& memory) {
  if (memory.address.support != MemoryOperandSupportKind::Prepared ||
      !memory.address.can_use_base_plus_offset) {
    return target_unsupported(bad_header(instruction) +
                              "memory address is not a prepared base+offset");
  }
  const auto address = memory_address(memory.address);
  if (address.empty()) {
    return target_unsupported(bad_header(instruction) + "memory address is not printable");
  }
  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "memory mnemonic is not printable");
  }

  if (memory.memory_kind == MemoryInstructionKind::Load) {
    if (!memory.result_register.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "load node is missing a structured destination register operand");
    }
    std::ostringstream out;
    out << mnemonic << " " << register_name(*memory.result_register) << ", " << address;
    return target_printed({out.str()});
  }
  if (!memory.value.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "store node is missing stored value operand");
  }
  const auto* value = std::get_if<RegisterOperand>(&memory.value->payload);
  if (memory.value->kind != OperandKind::Register || value == nullptr) {
    return target_unsupported(bad_header(instruction) + "store value is not a register operand");
  }
  std::ostringstream out;
  out << mnemonic << " " << register_name(*value) << ", " << address;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_atomic_memory(
    const InstructionRecord& instruction,
    const AtomicMemoryInstructionRecord& atomic) {
  const auto pointer = atomic_pointer_register_name(atomic);
  if (atomic.atomic_kind != AtomicMemoryInstructionKind::Fence && !pointer.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "atomic memory node is missing printable pointer register");
  }
  const auto address = pointer.has_value() ? "[" + *pointer + "]" : std::string{};

  switch (atomic.atomic_kind) {
    case AtomicMemoryInstructionKind::Load: {
      if (!atomic.result_register.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic load is missing result register");
      }
      const auto result = atomic_value_register_name(*atomic.result_register, atomic.width_bytes);
      const auto mnemonic = atomic.acquire_semantics
                                ? atomic_acquire_load_mnemonic(atomic.width_bytes)
                                : atomic_plain_load_mnemonic(atomic.width_bytes);
      if (!result.has_value() || mnemonic.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic load has unsupported width or register view");
      }
      std::ostringstream out;
      out << mnemonic << " " << *result << ", " << address;
      return target_printed({out.str()});
    }
    case AtomicMemoryInstructionKind::Store: {
      if (!atomic.stored_register.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic store is missing stored register");
      }
      const auto stored = atomic_value_register_name(*atomic.stored_register, atomic.width_bytes);
      const auto mnemonic = atomic.release_semantics
                                ? atomic_release_store_mnemonic(atomic.width_bytes)
                                : atomic_plain_store_mnemonic(atomic.width_bytes);
      if (!stored.has_value() || mnemonic.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic store has unsupported width or register view");
      }
      std::ostringstream out;
      out << mnemonic << " " << *stored << ", " << address;
      return target_printed({out.str()});
    }
    case AtomicMemoryInstructionKind::Fence:
      if (!atomic.memory_barrier_required) {
        return target_unsupported(bad_header(instruction) +
                                  "relaxed atomic fence is outside the printable subset");
      }
      return target_printed({"dmb ish"});
    case AtomicMemoryInstructionKind::RmwLoop: {
      if (!atomic.result_register.has_value() || !atomic.stored_register.has_value() ||
          !atomic.rmw_new_value_register.has_value() ||
          !atomic.exclusive_status_register.has_value() || !atomic.exclusive_retry_loop) {
        return target_unsupported(
            bad_header(instruction) +
            "atomic rmw loop is missing structured result, value, scratch, or status registers");
      }
      const auto old_value = atomic_value_register_name(*atomic.result_register,
                                                        atomic.width_bytes);
      const auto operand = atomic_value_register_name(*atomic.stored_register,
                                                      atomic.width_bytes);
      const auto new_value = atomic_value_register_name(*atomic.rmw_new_value_register,
                                                        atomic.width_bytes);
      const auto status = atomic_value_register_name(*atomic.exclusive_status_register, 4);
      const auto load = atomic_exclusive_load_mnemonic(atomic.width_bytes,
                                                       atomic.acquire_semantics);
      const auto store = atomic_exclusive_store_mnemonic(atomic.width_bytes,
                                                         atomic.release_semantics);
      const auto op = atomic_rmw_operation_mnemonic(atomic.rmw_opcode);
      if (!old_value.has_value() || !operand.has_value() || !new_value.has_value() ||
          !status.has_value() || load.empty() || store.empty() || op.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic rmw loop has unsupported width, opcode, or register view");
      }
      const auto retry = atomic_loop_label(atomic, "retry");
      std::vector<std::string> lines;
      lines.push_back(retry + ":");
      lines.push_back(std::string(load) + " " + *old_value + ", " + address);
      if (atomic.rmw_opcode == bir::AtomicRmwOpcode::Exchange) {
        lines.push_back(std::string(op) + " " + *new_value + ", " + *operand);
      } else {
        lines.push_back(std::string(op) + " " + *new_value + ", " + *old_value + ", " +
                        *operand);
      }
      lines.push_back(std::string(store) + " " + *status + ", " + *new_value + ", " +
                      address);
      lines.push_back("cbnz " + *status + ", " + retry);
      return target_printed(std::move(lines));
    }
    case AtomicMemoryInstructionKind::CompareExchangeLoop: {
      if (!atomic.expected_register.has_value() || !atomic.desired_register.has_value() ||
          !atomic.result_register.has_value() || !atomic.exclusive_status_register.has_value() ||
          !atomic.exclusive_retry_loop || !atomic.compare_exchange_failure_clears_monitor) {
        return target_unsupported(
            bad_header(instruction) +
            "atomic compare-exchange loop is missing structured operands, status, or monitor-clear facts");
      }
      const RegisterOperand* loaded_operand = nullptr;
      if (atomic.compare_exchange_result_is_old_value) {
        loaded_operand = &*atomic.result_register;
      } else if (atomic.compare_loaded_register.has_value()) {
        loaded_operand = &*atomic.compare_loaded_register;
      }
      if (loaded_operand == nullptr) {
        return target_unsupported(bad_header(instruction) +
                                  "atomic compare-exchange loop is missing loaded-value register");
      }
      const bool failure_acquire = atomic_ordering_has_acquire(atomic.failure_ordering);
      const bool success_needs_post_acquire =
          atomic.acquire_semantics && !failure_acquire;
      const auto loaded = atomic_value_register_name(*loaded_operand, atomic.width_bytes);
      const auto expected = atomic_value_register_name(*atomic.expected_register,
                                                       atomic.width_bytes);
      const auto desired = atomic_value_register_name(*atomic.desired_register,
                                                      atomic.width_bytes);
      const auto result = atomic_value_register_name(*atomic.result_register,
                                                     atomic.compare_exchange_result_is_boolean
                                                         ? 4
                                                         : atomic.width_bytes);
      const auto status = atomic_value_register_name(*atomic.exclusive_status_register, 4);
      const auto load = atomic_exclusive_load_mnemonic(atomic.width_bytes, failure_acquire);
      const auto store = atomic_exclusive_store_mnemonic(atomic.width_bytes,
                                                         atomic.release_semantics);
      if (!loaded.has_value() || !expected.has_value() || !desired.has_value() ||
          !result.has_value() || !status.has_value() || load.empty() || store.empty()) {
        return target_unsupported(
            bad_header(instruction) +
            "atomic compare-exchange loop has unsupported width or register view");
      }
      const auto retry = atomic_loop_label(atomic, "retry");
      const auto failure = atomic_loop_label(atomic, "failure");
      const auto done = atomic_loop_label(atomic, "done");
      std::vector<std::string> lines;
      lines.push_back(retry + ":");
      lines.push_back(std::string(load) + " " + *loaded + ", " + address);
      lines.push_back("cmp " + *loaded + ", " + *expected);
      lines.push_back("b.ne " + failure);
      lines.push_back(std::string(store) + " " + *status + ", " + *desired + ", " +
                      address);
      lines.push_back("cbnz " + *status + ", " + retry);
      if (success_needs_post_acquire) {
        lines.push_back("dmb ishld");
      }
      if (atomic.compare_exchange_result_is_boolean) {
        lines.push_back("mov " + *result + ", #1");
        lines.push_back("b " + done);
      }
      lines.push_back(failure + ":");
      lines.push_back("clrex");
      if (atomic.compare_exchange_result_is_boolean) {
        lines.push_back("mov " + *result + ", #0");
        lines.push_back(done + ":");
      }
      return target_printed(std::move(lines));
    }
  }
  return target_unsupported(bad_header(instruction) +
                            "atomic memory kind is outside the printable subset");
}

mir::TargetInstructionPrintResult print_frame(const InstructionRecord& instruction,
                                              const FrameInstructionRecord& frame) {
  if (frame.source_frame == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "frame node is missing prepared frame provenance");
  }
  if (frame.function_name == c4c::kInvalidFunctionName) {
    return target_unsupported(bad_header(instruction) +
                              "frame node is missing function identity");
  }
  if (frame.frame_alignment_bytes == 0) {
    return target_unsupported(bad_header(instruction) +
                              "frame node is missing prepared frame alignment");
  }
  if (frame.has_dynamic_stack) {
    return target_unsupported(bad_header(instruction) +
                              "dynamic-stack frame node is outside the printable subset");
  }
  if (!frame.saved_callee_registers.empty() || frame.callee_save.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "callee-save frame node is outside the printable subset");
  }

  if (frame.frame_kind != FrameInstructionKind::PrologueSetup &&
      frame.frame_kind != FrameInstructionKind::EpilogueTeardown) {
    return target_unsupported(bad_header(instruction) +
                              "frame node kind is outside the printable subset");
  }
  if (frame.frame_size_bytes == 0) {
    return target_printed({});
  }
  if (frame.frame_size_bytes > 4095) {
    return target_unsupported(bad_header(instruction) +
                              "frame adjustment immediate is outside the plain #imm "
                              "encoding range 0..4095");
  }

  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "frame mnemonic is not printable");
  }

  std::ostringstream out;
  out << mnemonic << " sp, sp, #" << frame.frame_size_bytes;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_scalar(const InstructionRecord& instruction,
                                               const ScalarInstructionRecord& scalar) {
  if (!scalar.result_register.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar node is missing a structured destination register operand");
  }
  if (scalar.scalar_cast.has_value()) {
    return print_scalar_cast_instruction(instruction, scalar, bad_header(instruction));
  }
  auto alu = make_scalar_alu_print_lines(instruction, scalar);
  if (alu.lines.has_value()) {
    return target_printed(std::move(*alu.lines));
  }
  return target_unsupported(bad_header(instruction) + alu.diagnostic);
}

mir::TargetInstructionPrintResult print_scalar_fp_unary_intrinsic(
    const InstructionRecord& instruction,
    const ScalarFpUnaryIntrinsicRecord& intrinsic) {
  if (instruction.family != InstructionFamily::Intrinsic ||
      instruction.opcode != MachineOpcode::ScalarFpUnaryIntrinsic) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires an intrinsic machine opcode");
  }
  if (intrinsic.source_carrier == nullptr ||
      intrinsic.source_carrier->carrier_kind !=
          prepare::PreparedIntrinsicCarrierKind::Complete) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires complete prepared carrier provenance");
  }
  if (intrinsic.family != bir::IntrinsicFamilyKind::ScalarFpUnary ||
      intrinsic.operation != bir::IntrinsicOperationKind::FAbs) {
    return target_unsupported(
        bad_header(instruction) +
        "intrinsic operation is outside the printable scalar FP unary subset");
  }
  if ((intrinsic.operand_type != bir::TypeKind::F32 &&
       intrinsic.operand_type != bir::TypeKind::F64) ||
      intrinsic.operand_type != intrinsic.result_type) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires matching F32/F64 operand and result types");
  }
  if (!intrinsic.has_prepared_call_plan) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires prepared call-plan authority");
  }
  if (intrinsic.has_side_effects || intrinsic.requires_feature) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer only supports side-effect-free feature-free fabs");
  }
  const auto* operand_register = std::get_if<RegisterOperand>(&intrinsic.operand.payload);
  if (!intrinsic.result_register.has_value() ||
      intrinsic.operand.kind != OperandKind::Register ||
      operand_register == nullptr) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer requires explicit operand and result registers");
  }
  const auto view = floating_register_view(intrinsic.operand_type);
  if (!view.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer has no register view for operand type");
  }
  const auto result = fp_register_name_with_view(*intrinsic.result_register, *view);
  const auto operand = fp_register_name_with_view(*operand_register, *view);
  if (!result.has_value() || !operand.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "scalar FP unary intrinsic printer has incomplete printable FPR register facts");
  }
  std::ostringstream out;
  out << "fabs " << *result << ", " << *operand;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_crc32w_intrinsic(
    const InstructionRecord& instruction,
    const Crc32WIntrinsicRecord& intrinsic) {
  if (instruction.family != InstructionFamily::Intrinsic ||
      instruction.opcode != MachineOpcode::Crc32WIntrinsic) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires a CRC32W machine opcode");
  }
  if (!has_complete_intrinsic_source(intrinsic.source_carrier)) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires complete prepared carrier provenance");
  }
  if (intrinsic.family != bir::IntrinsicFamilyKind::Crc ||
      intrinsic.operation != bir::IntrinsicOperationKind::Crc32W ||
      intrinsic.required_feature != bir::IntrinsicFeatureKind::AArch64Crc ||
      !intrinsic.requires_feature ||
      intrinsic.operand_type != bir::TypeKind::I32 ||
      intrinsic.result_type != bir::TypeKind::I32 ||
      intrinsic.signedness != bir::IntrinsicSignedness::Unsigned ||
      !has_exact_intrinsic_roles(intrinsic.operand_roles,
                                 {bir::IntrinsicOperandRole::Accumulator,
                                  bir::IntrinsicOperandRole::Data})) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires selected CRC32W carrier facts");
  }
  if (!intrinsic.has_prepared_call_plan || !intrinsic.result_register.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires prepared register authority");
  }
  const auto* accumulator = std::get_if<RegisterOperand>(&intrinsic.accumulator.payload);
  const auto* data = std::get_if<RegisterOperand>(&intrinsic.data.payload);
  if (intrinsic.accumulator.kind != OperandKind::Register ||
      intrinsic.data.kind != OperandKind::Register ||
      accumulator == nullptr || data == nullptr) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer requires explicit operand registers");
  }
  const auto result_name =
      register_name_with_view(*intrinsic.result_register, abi::RegisterView::W);
  const auto accumulator_name = register_name_with_view(*accumulator, abi::RegisterView::W);
  const auto data_name = register_name_with_view(*data, abi::RegisterView::W);
  if (!result_name.has_value() || !accumulator_name.has_value() ||
      !data_name.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "CRC32W intrinsic printer has incomplete printable W-register facts");
  }
  std::ostringstream out;
  out << "crc32w " << *result_name << ", " << *accumulator_name << ", " << *data_name;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_vector_load_intrinsic(
    const InstructionRecord& instruction,
    const VectorLoadIntrinsicRecord& intrinsic) {
  if (instruction.family != InstructionFamily::Intrinsic ||
      instruction.opcode != MachineOpcode::VectorLoadIntrinsic) {
    return target_unsupported(bad_header(instruction) +
                              "vector-load intrinsic printer requires a vector-load machine opcode");
  }
  if (!has_complete_intrinsic_source(intrinsic.source_carrier)) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-load intrinsic printer requires complete prepared carrier provenance");
  }
  if (intrinsic.family != bir::IntrinsicFamilyKind::VectorMemory ||
      intrinsic.operation != bir::IntrinsicOperationKind::VectorLoad ||
      intrinsic.required_feature != bir::IntrinsicFeatureKind::AArch64Neon ||
      !intrinsic.requires_feature ||
      intrinsic.operand_type != bir::TypeKind::Ptr ||
      intrinsic.result_type != bir::TypeKind::I128 ||
      intrinsic.memory_access != bir::IntrinsicMemoryAccessKind::Read ||
      !has_exact_intrinsic_roles(intrinsic.operand_roles,
                                 {bir::IntrinsicOperandRole::Pointer}) ||
      !has_v16i8_unsigned_shape(intrinsic.vector_element_type,
                                intrinsic.vector_element_width_bytes,
                                intrinsic.vector_lane_count,
                                intrinsic.vector_total_width_bytes,
                                intrinsic.signedness)) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-load intrinsic printer requires selected v16i8 load carrier facts");
  }
  const auto* pointer = std::get_if<RegisterOperand>(&intrinsic.pointer.payload);
  if (!intrinsic.has_prepared_call_plan || !intrinsic.result_register.has_value() ||
      intrinsic.pointer.kind != OperandKind::Register || pointer == nullptr ||
      intrinsic.memory.base_kind != MemoryBaseKind::PointerValue ||
      !intrinsic.memory.base_register.has_value() ||
      intrinsic.memory.byte_offset != 0 ||
      intrinsic.memory.size_bytes != intrinsic.vector_total_width_bytes ||
      intrinsic.memory.address_space != bir::AddressSpace::Default) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-load intrinsic printer requires pointer memory and result register authority");
  }
  const auto result_name = f128_vector_register_name(*intrinsic.result_register);
  const auto pointer_name = register_name_with_view(*pointer, abi::RegisterView::X);
  const auto memory_base_name =
      register_name_with_view(*intrinsic.memory.base_register, abi::RegisterView::X);
  if (!result_name.has_value() || !pointer_name.has_value() ||
      !memory_base_name.has_value() || *pointer_name != *memory_base_name) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-load intrinsic printer has incomplete printable vector or pointer register facts");
  }
  std::ostringstream out;
  out << "ld1 {" << *result_name << "}, [" << *memory_base_name << "]";
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_vector_add_intrinsic(
    const InstructionRecord& instruction,
    const VectorAddIntrinsicRecord& intrinsic) {
  if (instruction.family != InstructionFamily::Intrinsic ||
      instruction.opcode != MachineOpcode::VectorAddIntrinsic) {
    return target_unsupported(bad_header(instruction) +
                              "vector-add intrinsic printer requires a vector-add machine opcode");
  }
  if (!has_complete_intrinsic_source(intrinsic.source_carrier)) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-add intrinsic printer requires complete prepared carrier provenance");
  }
  if (intrinsic.family != bir::IntrinsicFamilyKind::VectorOperation ||
      intrinsic.operation != bir::IntrinsicOperationKind::VectorAdd ||
      intrinsic.required_feature != bir::IntrinsicFeatureKind::AArch64Neon ||
      !intrinsic.requires_feature ||
      intrinsic.operand_type != bir::TypeKind::I128 ||
      intrinsic.result_type != bir::TypeKind::I128 ||
      intrinsic.memory_access != bir::IntrinsicMemoryAccessKind::None ||
      !has_exact_intrinsic_roles(intrinsic.operand_roles,
                                 {bir::IntrinsicOperandRole::VectorLhs,
                                  bir::IntrinsicOperandRole::VectorRhs}) ||
      !has_v16i8_unsigned_shape(intrinsic.vector_element_type,
                                intrinsic.vector_element_width_bytes,
                                intrinsic.vector_lane_count,
                                intrinsic.vector_total_width_bytes,
                                intrinsic.signedness)) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-add intrinsic printer requires selected v16i8 add carrier facts");
  }
  const auto* lhs = std::get_if<RegisterOperand>(&intrinsic.lhs.payload);
  const auto* rhs = std::get_if<RegisterOperand>(&intrinsic.rhs.payload);
  if (!intrinsic.has_prepared_call_plan || !intrinsic.result_register.has_value() ||
      intrinsic.lhs.kind != OperandKind::Register ||
      intrinsic.rhs.kind != OperandKind::Register ||
      lhs == nullptr || rhs == nullptr) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-add intrinsic printer requires explicit operand and result registers");
  }
  const auto result_name = f128_vector_register_name(*intrinsic.result_register);
  const auto lhs_name = f128_vector_register_name(*lhs);
  const auto rhs_name = f128_vector_register_name(*rhs);
  if (!result_name.has_value() || !lhs_name.has_value() || !rhs_name.has_value()) {
    return target_unsupported(
        bad_header(instruction) +
        "vector-add intrinsic printer has incomplete printable vector register facts");
  }
  std::ostringstream out;
  out << "add " << *result_name << ", " << *lhs_name << ", " << *rhs_name;
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_return(const InstructionRecord& instruction,
                                               const ReturnInstructionRecord& ret) {
  std::vector<std::string> lines;
  const auto print_form = classify_return_value_print_form(ret);
  switch (print_form) {
    case ReturnValuePrintForm::PrimaryReturn: {
      const auto return_mnemonic = required_primary_mnemonic(instruction);
      if (return_mnemonic.empty()) {
        return target_unsupported(bad_header(instruction) + "return mnemonic is not printable");
      }
      return target_printed({std::string(return_mnemonic)});
    }
    case ReturnValuePrintForm::Unsupported:
      return target_unsupported(bad_header(instruction) +
                                "return value is not a printable immediate operand");
    case ReturnValuePrintForm::ImmediateMaterialization: {
      const auto* immediate = ret.value.has_value()
                                  ? std::get_if<ImmediateOperand>(&ret.value->payload)
                                  : nullptr;
      if (immediate == nullptr) {
        return target_unsupported(bad_header(instruction) +
                                  "return value is not a printable immediate operand");
      }
      if (!is_printable_return_immediate_materialization_value(*immediate)) {
        return target_unsupported(bad_header(instruction) +
                                  "return immediate is outside the selected printable subset");
      }
      const auto result_register = return_immediate_materialization_register(ret.value_type);
      if (!result_register.has_value()) {
        return target_unsupported(bad_header(instruction) +
                                  "return type is outside the selected printable subset");
      }
      const auto result_register_name = abi::register_name(*result_register);
      const auto move_mnemonic = required_auxiliary_mnemonic(instruction);
      if (move_mnemonic.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "return move mnemonic is not printable");
      }
      std::ostringstream move_line;
      move_line << move_mnemonic << " " << result_register_name << ", #"
                << immediate->signed_value;
      lines.push_back(move_line.str());
      break;
    }
    case ReturnValuePrintForm::NoValue:
      break;
  }
  const auto return_mnemonic = required_primary_mnemonic(instruction);
  if (return_mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "return mnemonic is not printable");
  }
  lines.emplace_back(return_mnemonic);
  return target_printed(std::move(lines));
}

}  // namespace

mir::TargetInstructionPrintResult MachineInstructionPrinter::print_instruction(
    const mir::MachinePrintContext&,
    const mir::MachineInstruction<InstructionRecord>& instruction) const {
  return print_machine_instruction_line_payloads(instruction.target);
}

mir::TargetInstructionPrintResult print_machine_instruction_line_payloads(
    const InstructionRecord& instruction) {
  if (const auto invalid = validate_selected_machine_node(instruction); invalid.has_value()) {
    return target_unsupported(*invalid);
  }

  if (const auto* spill_reload =
          std::get_if<SpillReloadInstructionRecord>(&instruction.payload)) {
    return print_spill_reload(instruction, *spill_reload);
  }
  if (const auto* branch = std::get_if<BranchInstructionRecord>(&instruction.payload)) {
    return print_branch(instruction, *branch);
  }
  if (const auto* memory = std::get_if<MemoryInstructionRecord>(&instruction.payload)) {
    return print_memory(instruction, *memory);
  }
  if (const auto* atomic =
          std::get_if<AtomicMemoryInstructionRecord>(&instruction.payload)) {
    return print_atomic_memory(instruction, *atomic);
  }
  if (const auto* frame = std::get_if<FrameInstructionRecord>(&instruction.payload)) {
    return print_frame(instruction, *frame);
  }
  if (const auto* move =
          std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.payload)) {
    return print_call_boundary_move(instruction, *move);
  }
  if (const auto* binding =
          std::get_if<CallBoundaryAbiBindingInstructionRecord>(&instruction.payload)) {
    return print_call_boundary_abi_binding(instruction, *binding);
  }
  if (const auto* call = std::get_if<CallInstructionRecord>(&instruction.payload)) {
    return print_call(instruction, *call);
  }
  if (const auto* address = std::get_if<AddressMaterializationRecord>(&instruction.payload)) {
    return print_address_materialization_instruction(instruction, *address);
  }
  if (const auto* transport = std::get_if<I128TransportRecord>(&instruction.payload)) {
    return print_i128_transport(instruction, *transport);
  }
  if (const auto* transport = std::get_if<F128TransportRecord>(&instruction.payload)) {
    return print_f128_transport(instruction, *transport);
  }
  if (const auto* pair = std::get_if<I128PairOperationRecord>(&instruction.payload)) {
    return print_i128_pair_operation(instruction, *pair);
  }
  if (const auto* shift = std::get_if<I128ShiftRecord>(&instruction.payload)) {
    return print_i128_shift(instruction, *shift);
  }
  if (const auto* compare = std::get_if<I128CompareRecord>(&instruction.payload)) {
    return print_i128_compare(instruction, *compare);
  }
  if (const auto* helper =
          std::get_if<I128RuntimeHelperBoundaryRecord>(&instruction.payload)) {
    return print_i128_runtime_helper(instruction, *helper);
  }
  if (const auto* helper =
          std::get_if<F128RuntimeHelperBoundaryRecord>(&instruction.payload)) {
    return print_f128_runtime_helper(instruction, *helper);
  }
  if (const auto* scalar = std::get_if<ScalarInstructionRecord>(&instruction.payload)) {
    return print_scalar(instruction, *scalar);
  }
  if (const auto* intrinsic =
          std::get_if<ScalarFpUnaryIntrinsicRecord>(&instruction.payload)) {
    return print_scalar_fp_unary_intrinsic(instruction, *intrinsic);
  }
  if (const auto* intrinsic = std::get_if<Crc32WIntrinsicRecord>(&instruction.payload)) {
    return print_crc32w_intrinsic(instruction, *intrinsic);
  }
  if (const auto* intrinsic = std::get_if<VectorLoadIntrinsicRecord>(&instruction.payload)) {
    return print_vector_load_intrinsic(instruction, *intrinsic);
  }
  if (const auto* intrinsic = std::get_if<VectorAddIntrinsicRecord>(&instruction.payload)) {
    return print_vector_add_intrinsic(instruction, *intrinsic);
  }
  if (const auto* assembler = std::get_if<AssemblerInstructionRecord>(&instruction.payload)) {
    return print_assembler(instruction, *assembler);
  }
  if (const auto* ret = std::get_if<ReturnInstructionRecord>(&instruction.payload)) {
    return print_return(instruction, *ret);
  }
  return target_unsupported(bad_header(instruction) +
                            "instruction family is not in the printable subset");
}

}  // namespace c4c::backend::aarch64::codegen
