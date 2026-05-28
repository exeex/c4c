#include "dispatch_publication.hpp"

#include "../../query.hpp"
#include "../../../prealloc/prepared_lookups.hpp"
#include "../abi/abi.hpp"
#include "alu.hpp"
#include "cast_ops.hpp"
#include "comparison.hpp"
#include "dispatch_lookup.hpp"
#include "dispatch_producers.hpp"
#include "dispatch_edge_copies.hpp"
#include "dispatch_value_materialization.hpp"
#include "float_ops.hpp"
#include "globals.hpp"
#include "instruction.hpp"
#include "machine_printer.hpp"
#include "memory.hpp"
#include "operands.hpp"
#include "prepared_value_home_materialization.hpp"
#include "variadic.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

[[nodiscard]] std::optional<c4c::ValueNameId> prepared_named_value_id(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  return prepare::resolve_prepared_value_name_id(context.function.prepared->names,
                                                 value.name);
}

}  // namespace

[[nodiscard]] bool registers_alias(const RegisterOperand& lhs,
                                   const RegisterOperand& rhs) {
  return lhs.reg.bank == rhs.reg.bank && lhs.reg.index == rhs.reg.index;
}
[[nodiscard]] std::optional<unsigned> integer_bit_width(bir::TypeKind type) {
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
    case bir::TypeKind::Ptr:
      return 64U;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}
[[nodiscard]] std::optional<unsigned> power_of_two_shift(std::uint64_t value) {
  if (value == 0 || (value & (value - 1U)) != 0) {
    return std::nullopt;
  }
  unsigned shift = 0;
  while (value > 1U) {
    value >>= 1U;
    ++shift;
  }
  return shift;
}
[[nodiscard]] const prepare::PreparedFrameSlot* find_frame_slot(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
}
[[nodiscard]] const prepare::PreparedStackObject* find_stack_object(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedObjectId object_id) {
  for (const auto& object : stack_layout.objects) {
    if (object.object_id == object_id) {
      return &object;
    }
  }
  return nullptr;
}
[[nodiscard]] std::optional<std::string> prepared_frame_slot_load_address(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  const auto* access =
      addressing != nullptr
          ? prepare::find_prepared_memory_access(
                *addressing, context.control_flow_block->block_label, instruction_index)
          : nullptr;
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return std::nullopt;
  }
  const auto* slot =
      find_frame_slot(context.function.prepared->stack_layout, *access->address.frame_slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }
  const auto offset =
      static_cast<std::int64_t>(slot->offset_bytes) + access->address.byte_offset;
  std::string address =
      context.function.frame_plan != nullptr &&
              context.function.frame_plan->uses_frame_pointer_for_fixed_slots
          ? "[x29"
          : "[sp";
  if (offset != 0) {
    address += ", #";
    address += std::to_string(offset);
  }
  address += "]";
  return address;
}
[[nodiscard]] std::string relocation_operand(std::string_view label,
                                             std::size_t byte_offset) {
  std::string operand{label};
  if (byte_offset != 0) {
    operand += "+";
    operand += std::to_string(byte_offset);
  }
  return operand;
}
[[nodiscard]] std::string register_indirect_address(std::string_view base,
                                                    std::size_t byte_offset) {
  std::string address{"["};
  address += base;
  if (byte_offset != 0) {
    address += ", #";
    address += std::to_string(byte_offset);
  }
  address += "]";
  return address;
}
[[nodiscard]] bool fixed_slots_use_frame_pointer(
    const module::FunctionLoweringContext& context) {
  return context.frame_plan != nullptr &&
         context.frame_plan->uses_frame_pointer_for_fixed_slots;
}
[[nodiscard]] std::string frame_slot_address(std::size_t offset_bytes,
                                             std::string_view base_register) {
  std::string address{"["};
  address += base_register;
  if (offset_bytes != 0) {
    address += ", #";
    address += std::to_string(offset_bytes);
  }
  address += "]";
  return address;
}
[[nodiscard]] std::string frame_slot_address(
    const module::FunctionLoweringContext& context,
    std::size_t offset_bytes) {
  return frame_slot_address(offset_bytes,
                            fixed_slots_use_frame_pointer(context) ? "x29" : "sp");
}
[[nodiscard]] std::optional<abi::RegisterView> scalar_view_for_type(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return abi::RegisterView::W;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return abi::RegisterView::X;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}
[[nodiscard]] std::optional<std::string> gp_register_name(std::uint8_t index,
                                                          abi::RegisterView view) {
  auto reg = abi::gp_register(index, view);
  if (!reg.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*reg);
}
[[nodiscard]] std::optional<std::string_view> scalar_load_mnemonic(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::string_view{"ldrb"};
    case bir::TypeKind::I16:
      return std::string_view{"ldrh"};
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return std::string_view{"ldr"};
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}
[[nodiscard]] std::optional<std::size_t> dispatch_publication_scalar_type_size_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::size_t{1};
    case bir::TypeKind::I16:
      return std::size_t{2};
    case bir::TypeKind::I32:
      return std::size_t{4};
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return std::size_t{8};
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}
[[nodiscard]] std::optional<std::string_view> scalar_load_mnemonic_for_width(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return std::string_view{"ldrb"};
    case 2:
      return std::string_view{"ldrh"};
    case 4:
    case 8:
      return std::string_view{"ldr"};
  }
  return std::nullopt;
}
[[nodiscard]] std::optional<std::string_view> scalar_store_mnemonic(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::string_view{"strb"};
    case bir::TypeKind::I16:
      return std::string_view{"strh"};
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return std::string_view{"str"};
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}
[[nodiscard]] std::optional<unsigned> scalar_integer_width_bits(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return 64U;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}
[[nodiscard]] std::optional<abi::RegisterReference> scalar_gp_register_view(
    abi::RegisterReference reg,
    bir::TypeKind type) {
  const auto width_bits = scalar_integer_width_bits(type);
  if (!width_bits.has_value()) {
    return std::nullopt;
  }
  if (*width_bits == 32U) {
    return abi::gp_register(reg.index, abi::RegisterView::W);
  }
  return abi::gp_register(reg.index, abi::RegisterView::X);
}
[[nodiscard]] std::optional<abi::RegisterReference> scalar_fp_register_view(
    abi::RegisterReference reg,
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return abi::fp_simd_register(reg.index, abi::RegisterView::S);
    case bir::TypeKind::F64:
      return abi::fp_simd_register(reg.index, abi::RegisterView::D);
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

[[nodiscard]] std::optional<std::size_t> prepared_frame_address_offset_for_value(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name,
    std::optional<std::size_t> before_or_at_instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.address_materialization_lookups == nullptr ||
      context.control_flow_block == nullptr ||
      value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto resolved =
      prepare::find_indexed_prepared_frame_address_offset_for_value(
          context.function.prepared->stack_layout,
          context.function.address_materialization_lookups,
          context.control_flow_block->block_label,
          value_name,
          before_or_at_instruction_index);
  return resolved.has_value()
             ? std::optional<std::size_t>{resolved->stack_offset_bytes}
             : std::nullopt;
}
[[nodiscard]] std::optional<std::size_t> local_slot_address_frame_offset(
    const module::BlockLoweringContext&,
    std::string_view) {
  return std::nullopt;
}
[[nodiscard]] std::optional<std::size_t> local_aggregate_address_frame_offset(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name) {
  return prepared_frame_address_offset_for_value(context, value_name, std::nullopt);
}
[[nodiscard]] bool emit_local_slot_address_publication_to_register_impl(
    const module::BlockLoweringContext& context,
    const bir::BinaryInst& binary,
    std::uint8_t target_index,
    std::optional<std::size_t> instruction_index,
    std::vector<std::string>& lines) {
  if (binary.result.type != bir::TypeKind::Ptr ||
      binary.operand_type != bir::TypeKind::Ptr ||
      (binary.opcode != bir::BinaryOpcode::Add &&
       binary.opcode != bir::BinaryOpcode::Sub) ||
      binary.lhs.kind != bir::Value::Kind::Named ||
      binary.rhs.kind != bir::Value::Kind::Immediate) {
    return false;
  }
  const auto lhs_name = prepared_named_value_id(context, binary.lhs);
  const auto base_offset =
      lhs_name.has_value()
          ? prepared_frame_address_offset_for_value(
                context, *lhs_name, instruction_index)
          : std::nullopt;
  if (!base_offset.has_value()) {
    return false;
  }
  const auto target_register = abi::gp_register(target_index, abi::RegisterView::X);
  const auto target = target_register.has_value()
                          ? std::optional<std::string>{
                                std::string{abi::register_name(*target_register)}}
                          : std::nullopt;
  if (!target.has_value()) {
    return false;
  }
  const std::int64_t signed_base =
      static_cast<std::int64_t>(*base_offset);
  const std::int64_t signed_delta =
      binary.opcode == bir::BinaryOpcode::Add ? binary.rhs.immediate
                                              : -binary.rhs.immediate;
  const std::int64_t adjusted_offset = signed_base + signed_delta;
  if (adjusted_offset < 0) {
    return false;
  }
  const std::string_view base =
      context.function.frame_plan != nullptr &&
              context.function.frame_plan->uses_frame_pointer_for_fixed_slots
          ? "x29"
          : "sp";
  lines.push_back("add " + *target + ", " + std::string{base} + ", #" +
                  std::to_string(adjusted_offset));
  return true;
}
[[nodiscard]] bool emit_local_slot_address_publication_to_register(
    const module::BlockLoweringContext& context,
    const bir::BinaryInst& binary,
    std::uint8_t target_index,
    std::optional<std::size_t> before_or_at_instruction_index,
    std::vector<std::string>& lines) {
  return emit_local_slot_address_publication_to_register_impl(
      context, binary, target_index, before_or_at_instruction_index, lines);
}
[[nodiscard]] std::optional<module::MachineInstruction>
lower_local_slot_address_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr || binary->result.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  auto result_register = make_named_prepared_result_register(context, binary->result);
  std::optional<std::size_t> result_stack_offset_bytes;
  if (!result_register.has_value()) {
    const auto* home = prepared_value_home_for_value(context, binary->result);
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (home == nullptr ||
        home->kind != prepare::PreparedValueHomeKind::StackSlot ||
        !home->offset_bytes.has_value() ||
        scratches.empty()) {
      return std::nullopt;
    }
    const auto scratch = abi::gp_register(scratches.front().index, abi::RegisterView::X);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    result_register = RegisterOperand{
        .reg = *scratch,
        .role = RegisterOperandRole::SpillAuthority,
        .value_id = home->value_id,
        .value_name = home->value_name,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = abi::RegisterView::X,
    };
    result_stack_offset_bytes = *home->offset_bytes;
  }
  if (!abi::is_gp_register(result_register->reg)) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  if (!emit_local_slot_address_publication_to_register_impl(
          context, *binary, result_register->reg.index, instruction_index, lines)) {
    return std::nullopt;
  }
  if (result_stack_offset_bytes.has_value()) {
    const auto result_name =
        gp_register_name(result_register->reg.index, abi::RegisterView::X);
    if (!result_name.has_value()) {
      return std::nullopt;
    }
    lines.push_back("str " + *result_name + ", " +
                    frame_slot_address(context.function, *result_stack_offset_bytes));
  }
  record_emitted_scalar_register(
      scalar_state, result_register->value_name, *result_register);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}
[[nodiscard]] bool register_operands_share_physical_register(
    const RegisterOperand& lhs,
    const RegisterOperand& rhs) {
  return lhs.reg.bank == rhs.reg.bank && lhs.reg.index == rhs.reg.index;
}
[[nodiscard]] std::optional<std::size_t> prepared_local_load_offset(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  const auto* access =
      addressing != nullptr
          ? prepare::find_prepared_memory_access(
                *addressing, context.control_flow_block->block_label, instruction_index)
          : nullptr;
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return std::nullopt;
  }
  const auto* slot =
      find_frame_slot(context.function.prepared->stack_layout,
                      *access->address.frame_slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }
  return slot->offset_bytes + static_cast<std::size_t>(access->address.byte_offset);
}
[[nodiscard]] std::optional<std::size_t> publication_parse_va_list_field_suffix(
    std::string_view base,
    std::string_view slot_name) {
  if (slot_name.size() <= base.size() + 1 ||
      slot_name.substr(0, base.size()) != base ||
      slot_name[base.size()] != '.') {
    return std::nullopt;
  }
  std::size_t value = 0;
  for (std::size_t index = base.size() + 1; index < slot_name.size(); ++index) {
    const char ch = slot_name[index];
    if (ch < '0' || ch > '9') {
      return std::nullopt;
    }
    value = value * 10U + static_cast<std::size_t>(ch - '0');
  }
  return value;
}
[[nodiscard]] std::optional<std::string> prepared_va_list_field_address(
    const module::BlockLoweringContext& context,
    std::string_view slot_name) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr) {
    return std::nullopt;
  }
  const auto* entry_plan =
      prepare::find_prepared_variadic_entry_plan(
          *context.function.prepared,
          context.function.control_flow->function_name);
  if (entry_plan == nullptr) {
    return std::nullopt;
  }
  for (const auto& homes : entry_plan->helper_operand_homes) {
    if (homes.helper != prepare::PreparedVariadicEntryHelperKind::VaStart ||
        !homes.destination_va_list.has_value()) {
      continue;
    }
    const auto& va_list_home = *homes.destination_va_list;
    const auto base_name =
        prepare::prepared_value_name(context.function.prepared->names,
                                     va_list_home.value_name);
    const auto field_offset = publication_parse_va_list_field_suffix(base_name, slot_name);
    if (!field_offset.has_value() ||
        va_list_home.kind != prepare::PreparedValueHomeKind::Register ||
        !va_list_home.register_name.has_value()) {
      continue;
    }
    const auto parsed = abi::parse_aarch64_register_name(*va_list_home.register_name);
    if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
      continue;
    }
    const auto base = abi::gp_register(parsed->index, abi::RegisterView::X);
    if (!base.has_value()) {
      continue;
    }
    return register_indirect_address(abi::register_name(*base), *field_offset);
  }
  return std::nullopt;
}
[[nodiscard]] std::uint64_t immediate_integer_bits(const bir::Value& value,
                                                   unsigned width_bits) {
  if (value.immediate_bits != 0U) {
    return width_bits == 32U ? static_cast<std::uint32_t>(value.immediate_bits)
                             : value.immediate_bits;
  }
  return width_bits == 32U ? static_cast<std::uint32_t>(value.immediate)
                           : static_cast<std::uint64_t>(value.immediate);
}
[[nodiscard]] bool is_byval_formal_value_name(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name) {
  if (context.function.prepared == nullptr ||
      context.function.bir_function == nullptr) {
    return false;
  }
  const std::string_view name =
      prepare::prepared_value_name(context.function.prepared->names, value_name);
  if (name.empty()) {
    return false;
  }
  for (const auto& param : context.function.bir_function->params) {
    if (param.is_byval && param.name == name) {
      return true;
    }
  }
  return false;
}
[[nodiscard]] const prepare::PreparedValueHome* prepared_value_home_for_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.value_locations == nullptr) {
    return nullptr;
  }
  const auto value_name = prepared_named_value_id(context, value);
  return value_name.has_value()
             ? prepare::find_indexed_prepared_value_home(
                   context.function.value_home_lookups,
                   context.function.regalloc,
                   context.function.value_locations,
                   *value_name)
             : nullptr;
}
[[nodiscard]] std::vector<prepare::PreparedBlockEntryPublication>
collect_current_block_entry_publications(const module::BlockLoweringContext& context) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return {};
  }
  return prepare::collect_prepared_block_entry_publications(
      context.function.value_locations, context.control_flow_block->block_label);
}
[[nodiscard]] bool value_has_current_block_entry_publication(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& home) {
  for (const auto& publication : collect_current_block_entry_publications(context)) {
    if (prepare::prepared_block_entry_publication_available(publication) &&
        publication.destination_kind == prepare::PreparedMoveDestinationKind::Value &&
        publication.destination_value_id == home.value_id) {
      return true;
    }
  }
  return false;
}
[[nodiscard]] const prepare::PreparedValueHome*
current_block_entry_publication_home(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return nullptr;
  }

  std::optional<prepare::PreparedValueId> value_id;
  if (context.function.value_home_lookups != nullptr) {
    const auto value_id_it =
        context.function.value_home_lookups->value_ids.find(*value_name);
    if (value_id_it == context.function.value_home_lookups->value_ids.end()) {
      return nullptr;
    }
    value_id = value_id_it->second;
  } else {
    value_id = prepare::find_prepared_value_id(context.function.regalloc,
                                               context.function.value_locations,
                                               *value_name);
  }
  return value_id.has_value()
             ? prepare::find_indexed_prepared_value_home(
                   context.function.value_home_lookups,
                   context.function.value_locations,
                   *value_id)
             : nullptr;
}
[[nodiscard]] std::optional<RegisterOperand> current_block_entry_publication_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    abi::RegisterView expected_view) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr ||
      value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto* home = current_block_entry_publication_home(context, value);
  if (home == nullptr) {
    return std::nullopt;
  }
  for (const auto& publication : collect_current_block_entry_publications(context)) {
    if (publication.destination_value_id != home->value_id ||
        !prepare::prepared_block_entry_publication_available(publication) ||
        !publication.destination_register_name.has_value()) {
      continue;
    }
    const auto parsed = abi::parse_aarch64_register_name(
        *publication.destination_register_name);
    if (!parsed.has_value() ||
        parsed->bank != abi::RegisterBank::GeneralPurpose) {
      continue;
    }
    auto reg = abi::gp_register(parsed->index, expected_view).value_or(*parsed);
    reg.view = expected_view;
    return RegisterOperand{
        .reg = reg,
        .role = RegisterOperandRole::StoragePlan,
        .value_id = home->value_id,
        .value_name = home->value_name,
        .expected_view = expected_view,
    };
  }
  return std::nullopt;
}
void record_current_block_entry_publication_registers(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state) {
  for (const auto& publication : collect_current_block_entry_publications(context)) {
    if (!prepare::prepared_block_entry_publication_available(publication) ||
        publication.home == nullptr ||
        !publication.destination_register_name.has_value()) {
      continue;
    }
    const auto parsed = abi::parse_aarch64_register_name(
        *publication.destination_register_name);
    if (!parsed.has_value() ||
        parsed->bank != abi::RegisterBank::GeneralPurpose) {
      continue;
    }
    record_emitted_scalar_register(
        scalar_state,
        publication.home->value_name,
        RegisterOperand{
            .reg = *parsed,
            .role = RegisterOperandRole::StoragePlan,
            .value_id = publication.home->value_id,
            .value_name = publication.home->value_name,
            .expected_view = parsed->view,
        });
  }
}

[[nodiscard]] std::optional<prepare::PreparedEdgePublicationSourceProducer>
prepared_publication_source_producer_for_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  if (context.function.prepared_lookups != nullptr) {
    const auto* producer =
        prepare::find_indexed_prepared_edge_publication_source_producer(
            &context.function.prepared_lookups->edge_publication_source_producers,
            *value_name);
    return producer != nullptr
               ? std::optional<prepare::PreparedEdgePublicationSourceProducer>{
                     *producer}
               : std::nullopt;
  }
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr) {
    return std::nullopt;
  }
  const auto source_producers =
      prepare::make_prepared_edge_publication_source_producer_lookups(
          *context.function.prepared,
          *context.function.control_flow);
  const auto* producer =
      prepare::find_indexed_prepared_edge_publication_source_producer(
          &source_producers,
          *value_name);
  return producer != nullptr
             ? std::optional<prepare::PreparedEdgePublicationSourceProducer>{
                   *producer}
             : std::nullopt;
}

[[nodiscard]] const bir::Inst* prepared_source_producer_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublicationSourceProducer& producer) {
  if (context.bir_block == nullptr || context.control_flow_block == nullptr ||
      producer.block_label != context.control_flow_block->block_label ||
      producer.instruction_index >= context.bir_block->insts.size() ||
      producer.kind == prepare::PreparedEdgePublicationSourceProducerKind::Unknown ||
      producer.kind == prepare::PreparedEdgePublicationSourceProducerKind::Immediate) {
    return nullptr;
  }
  return &context.bir_block->insts[producer.instruction_index];
}
[[nodiscard]] const bir::Value* prepared_source_producer_result(
    const prepare::PreparedEdgePublicationSourceProducer& producer) {
  switch (producer.kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return producer.load_local != nullptr ? &producer.load_local->result : nullptr;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return producer.load_global != nullptr ? &producer.load_global->result : nullptr;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return producer.cast != nullptr ? &producer.cast->result : nullptr;
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return producer.binary != nullptr ? &producer.binary->result : nullptr;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return producer.select != nullptr ? &producer.select->result : nullptr;
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return nullptr;
  }
  return nullptr;
}
[[nodiscard]] bool prepared_source_producer_matches_instruction(
    const prepare::PreparedEdgePublicationSourceProducer& producer,
    const bir::Inst& inst,
    const bir::Value& value) {
  const auto* result = prepared_source_producer_result(producer);
  if (result == nullptr ||
      result->kind != bir::Value::Kind::Named ||
      value.kind != bir::Value::Kind::Named ||
      result->name != value.name ||
      result->type != value.type) {
    return false;
  }
  switch (producer.kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return producer.load_local == std::get_if<bir::LoadLocalInst>(&inst);
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return producer.load_global == std::get_if<bir::LoadGlobalInst>(&inst);
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return producer.cast == std::get_if<bir::CastInst>(&inst);
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return producer.binary == std::get_if<bir::BinaryInst>(&inst);
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return producer.select == std::get_if<bir::SelectInst>(&inst);
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return false;
  }
  return false;
}
[[nodiscard]] const bir::Inst* prepared_source_producer_instruction_for_value(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublicationSourceProducer& producer,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  const auto* inst = prepared_source_producer_instruction(context, producer);
  if (inst == nullptr ||
      producer.instruction_index >= before_instruction_index ||
      !prepared_source_producer_matches_instruction(producer, *inst, value)) {
    return nullptr;
  }
  return inst;
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_missing_conditional_branch_condition_publication(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (context.bir_block == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch ||
      context.bir_block->terminator.kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto& condition = context.bir_block->terminator.condition;
  const auto condition_name = prepared_named_value_id(context, condition);
  if (!condition_name.has_value() ||
      find_emitted_scalar_register(scalar_state, *condition_name).has_value()) {
    return std::nullopt;
  }
  const auto* branch_condition =
      context.function.control_flow != nullptr
          ? prepare::find_prepared_branch_condition(
                *context.function.control_flow,
                context.control_flow_block->block_label)
          : nullptr;
  if (branch_condition == nullptr ||
      branch_condition->condition_value.kind != bir::Value::Kind::Named ||
      branch_condition->condition_value.name != condition.name) {
    return std::nullopt;
  }
  const auto producer =
      prepared_publication_source_producer_for_value(context,
                                                     branch_condition->condition_value);
  const auto* producer_inst =
      producer.has_value() ? prepared_source_producer_instruction(context, *producer)
                           : nullptr;
  if (!producer.has_value() || producer_inst == nullptr) {
    return std::nullopt;
  }
  return lower_scalar_control_value_instruction(
      context,
      *producer_inst,
      producer->instruction_index,
      scalar_state,
      diagnostics,
      true);
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_missing_fused_compare_operand_publication(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics,
    std::optional<std::uint8_t> preferred_target_index) {
  if (context.bir_block == nullptr || value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto* home =
      context.function.value_locations != nullptr
          ? prepare::find_indexed_prepared_value_home(
                context.function.value_home_lookups,
                context.function.regalloc,
                context.function.value_locations,
                *value_name)
          : nullptr;
  if (home == nullptr) {
    return std::nullopt;
  }
  if (find_emitted_scalar_register(scalar_state, *value_name).has_value() &&
      home->kind != prepare::PreparedValueHomeKind::StackSlot) {
    return std::nullopt;
  }
  auto resolved =
      resolve_value_operand(home->value_id, context.function, diagnostics);
  const auto expected_view = scalar_view_for_type(value.type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  if (auto published =
          current_block_entry_publication_register(context, value, *expected_view)) {
    record_emitted_scalar_register(scalar_state,
                                   published->value_name,
                                   *published);
    return std::nullopt;
  }

  std::uint8_t target_index = 0;
  std::uint8_t scratch_index = 0;
  bool has_target = false;
  if (resolved.has_value() && resolved->register_reference.has_value() &&
      abi::is_gp_register(*resolved->register_reference)) {
    target_index = resolved->register_reference->index;
    has_target = true;
  } else {
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    auto scratch_is_occupied = [&](const abi::RegisterReference& scratch) {
      for (const auto& [_, emitted] : scalar_state.emitted_registers) {
        if (emitted.reg.bank == scratch.bank && emitted.reg.index == scratch.index) {
          return true;
        }
      }
      return false;
    };
    if (preferred_target_index.has_value()) {
      for (const auto scratch : scratches) {
        if (scratch.index == *preferred_target_index &&
            !scratch_is_occupied(scratch)) {
          target_index = scratch.index;
          has_target = true;
          break;
        }
      }
    }
    for (const auto scratch : scratches) {
      if (!has_target && !scratch_is_occupied(scratch)) {
        target_index = scratch.index;
        has_target = true;
        break;
      }
    }
  }
  if (!has_target) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  for (const auto scratch : scratches) {
    if (scratch.index != target_index) {
      scratch_index = scratch.index;
      break;
    }
  }
  if (scratch_index == target_index) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  const auto producer = prepared_publication_source_producer_for_value(context, value);
  const auto* producer_inst =
      producer.has_value() ? prepared_source_producer_instruction(context, *producer)
                           : nullptr;
  if (producer_inst != nullptr) {
    if (!emit_value_publication_to_register(context,
                                            value,
                                            context.bir_block->insts.size(),
                                            target_index,
                                            scratch_index,
                                            lines,
                                            true) ||
        lines.empty()) {
      return std::nullopt;
    }
  } else {
    const auto plan = prepare::plan_prepared_scalar_publication(
        prepare::PreparedScalarPublicationInputs{
            .source_value = &value,
            .destination_home = home,
        });
    if (!prepare::prepared_scalar_publication_available(plan) ||
        (plan.hook_kind != prepare::PreparedScalarPublicationHookKind::RegisterHome &&
         plan.hook_kind != prepare::PreparedScalarPublicationHookKind::StackSlotHome) ||
        !emit_prepared_value_home_publication_to_register(context,
                                                          value,
                                                          *home,
                                                          target_index,
                                                          lines) ||
        lines.empty()) {
      return std::nullopt;
    }
  }
  auto reg = abi::x_register(target_index);
  if (resolved.has_value() && resolved->register_reference.has_value()) {
    reg = *resolved->register_reference;
  } else {
    reg = abi::gp_register(target_index, *expected_view).value_or(reg);
  }
  reg.view = *expected_view;
  RegisterOperand emitted{
      .reg = reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = home->value_id,
      .value_name = home->value_name,
      .expected_view = expected_view,
  };
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, context.bir_block->insts.size(), std::move(lines));
}

[[nodiscard]] std::vector<module::MachineInstruction>
lower_missing_fused_compare_operand_publications(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return lowered;
  }
  const auto* branch_condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (branch_condition == nullptr ||
      branch_condition->kind != prepare::PreparedBranchConditionKind::FusedCompare ||
      !branch_condition->can_fuse_with_branch) {
    return lowered;
  }
  if (branch_condition->lhs.has_value()) {
    std::optional<std::uint8_t> preferred_target_index;
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    const auto producer =
        prepared_publication_source_producer_for_value(context, *branch_condition->lhs);
    if (producer.has_value() &&
        producer->kind ==
            prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization &&
        scratches.size() > 1U) {
      preferred_target_index = scratches[1].index;
    }
    if (auto lhs = lower_missing_fused_compare_operand_publication(
            context,
            *branch_condition->lhs,
            scalar_state,
            diagnostics,
            preferred_target_index)) {
      lowered.push_back(std::move(*lhs));
    }
  }
  if (branch_condition->rhs.has_value()) {
    std::optional<std::uint8_t> preferred_target_index;
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    const auto producer =
        prepared_publication_source_producer_for_value(context, *branch_condition->rhs);
    if (producer.has_value() &&
        producer->kind ==
            prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization &&
        scratches.size() > 1U) {
      preferred_target_index = scratches[1].index;
    }
    if (auto rhs = lower_missing_fused_compare_operand_publication(
            context,
            *branch_condition->rhs,
            scalar_state,
            diagnostics,
            preferred_target_index)) {
      lowered.push_back(std::move(*rhs));
    }
  }
  return lowered;
}

void record_address_materialization_result(
    BlockScalarLoweringState& scalar_state,
    const module::MachineInstruction& instruction) {
  const auto* address_record =
      std::get_if<AddressMaterializationRecord>(&instruction.target.payload);
  if (address_record == nullptr || !address_record->result_register.has_value()) {
    return;
  }
  record_emitted_scalar_register(scalar_state,
                                 address_record->result_value_name,
                                 *address_record->result_register);
}
void record_memory_result(BlockScalarLoweringState& scalar_state,
                          const module::MachineInstruction& instruction) {
  const auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->result_stack_offset_bytes.has_value() ||
      !memory_record->result_register.has_value()) {
    return;
  }
  record_emitted_scalar_register(scalar_state,
                                 memory_record->result_value_name,
                                 *memory_record->result_register);
}
bool memory_load_result_feeds_before_return_fpr_abi(
    const module::BlockLoweringContext& context,
    prepare::PreparedValueId value_id) {
  return prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank(
             context.function.move_bundle_lookups,
             context.function.value_locations,
             context.block_index,
             value_id,
             prepare::PreparedRegisterBank::Fpr) != nullptr;
}
[[nodiscard]] const prepare::PreparedStoragePlanValue* find_storage_plan_value(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    prepare::PreparedValueId value_id) {
  for (const auto& value : storage_plan.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}
[[nodiscard]] bool symbol_fp_load_has_explicit_storage_placement(
    const module::BlockLoweringContext& context,
    const MemoryInstructionRecord& memory_record) {
  if (memory_record.address.base_kind != MemoryBaseKind::Symbol ||
      (memory_record.value_type != bir::TypeKind::F32 &&
       memory_record.value_type != bir::TypeKind::F64) ||
      !memory_record.result_value_id.has_value() ||
      !memory_record.result_register.has_value() ||
      !abi::is_fp_simd_register(memory_record.result_register->reg) ||
      context.function.storage_plan == nullptr) {
    return false;
  }
  const auto* storage =
      find_storage_plan_value(*context.function.storage_plan,
                              *memory_record.result_value_id);
  return storage != nullptr &&
         storage->encoding == prepare::PreparedStorageEncodingKind::Register &&
         storage->bank == prepare::PreparedRegisterBank::Fpr &&
         storage->register_placement.has_value();
}
void retarget_memory_result_to_prepared_home(
    const module::BlockLoweringContext& context,
    module::MachineInstruction& instruction) {
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  const bool frame_slot_return_publication =
      memory_record != nullptr &&
      memory_record->address.base_kind == MemoryBaseKind::FrameSlot &&
      memory_record->result_value_id.has_value() &&
      memory_load_result_feeds_before_return_fpr_abi(
          context, *memory_record->result_value_id);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Load ||
      (memory_record->address.base_kind != MemoryBaseKind::Symbol &&
       !frame_slot_return_publication) ||
      !memory_record->result_value_id.has_value() ||
      !memory_record->result_register.has_value() ||
      context.function.value_locations == nullptr) {
    return;
  }
  if (symbol_fp_load_has_explicit_storage_placement(context, *memory_record)) {
    return;
  }

  const auto* home =
      prepare::find_indexed_prepared_value_home(
          context.function.value_home_lookups,
          context.function.value_locations,
          *memory_record->result_value_id);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::Register ||
      !home->register_name.has_value()) {
    return;
  }

  const auto expected_view = memory_record->result_register->expected_view;
  const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
  if (!parsed.has_value() ||
      parsed->bank != memory_record->result_register->reg.bank) {
    return;
  }
  auto viewed = *parsed;
  if (expected_view.has_value()) {
    viewed.view = *expected_view;
  }
  memory_record->result_register->reg = viewed;
  memory_record->result_register->value_id = home->value_id;
  memory_record->result_register->value_name = home->value_name;
  memory_record->result_register->occupied_register_references = {viewed};
  memory_record->result_register->occupied_registers = {abi::register_name(viewed)};
}
void retarget_pointer_store_value_to_materialized_address(
    module::MachineInstruction& instruction,
    const RegisterOperand& materialized_address) {
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      memory_record->value_type != bir::TypeKind::Ptr) {
    return;
  }
  memory_record->value = make_register_operand(materialized_address);
}
void retarget_store_address_to_materialized_pointer(
    const bir::StoreLocalInst& store,
    module::MachineInstruction& instruction,
    const RegisterOperand& materialized_address) {
  if (!store.address.has_value() ||
      store.address->base_kind != bir::MemoryAddress::BaseKind::PointerValue) {
    return;
  }
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store) {
    return;
  }

  memory_record->address.base_kind = MemoryBaseKind::Register;
  memory_record->address.base_register = materialized_address;
  memory_record->address.frame_slot_id.reset();
  memory_record->address.symbol_name.reset();
  memory_record->address.symbol_label.clear();
  memory_record->address.pointer_value_name.reset();
  memory_record->address.pointer_value_id.reset();
  memory_record->address.byte_offset = store.address->byte_offset;
  memory_record->address.byte_offset_is_prepared_snapshot = false;
  memory_record->address.size_bytes = store.address->size_bytes;
  memory_record->address.align_bytes = store.address->align_bytes;
  memory_record->address.address_space = store.address->address_space;
  memory_record->address.is_volatile = store.address->is_volatile;
  memory_record->address.can_use_base_plus_offset = true;
}
void retarget_pointer_store_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      store->value.kind != bir::Value::Kind::Named ||
      store->value.type != bir::TypeKind::Ptr) {
    return;
  }
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      memory_record->value_type != bir::TypeKind::Ptr) {
    return;
  }
  const auto value_name = prepared_named_value_id(context, store->value);
  if (!value_name.has_value()) {
    return;
  }
  const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
  if (!emitted.has_value()) {
    return;
  }
  memory_record->value = make_register_operand(*emitted);
}
void retarget_store_local_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      !store_local_uses_pointer_value_address(*store) ||
      store->value.kind != bir::Value::Kind::Named) {
    return;
  }
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store) {
    return;
  }
  const auto* current_register =
      memory_record->value.has_value()
          ? std::get_if<RegisterOperand>(&memory_record->value->payload)
          : nullptr;
  if (current_register == nullptr) {
    return;
  }
  const auto value_register =
      prepared_or_emitted_store_value_register(context, store->value, scalar_state);
  if (!value_register.has_value()) {
    return;
  }
  if (register_operands_share_physical_register(*current_register, *value_register)) {
    return;
  }
  memory_record->value = make_register_operand(*value_register);
}
[[nodiscard]] std::optional<std::string_view> fixed_formal_scalar_store_mnemonic(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::string_view{"strb"};
    case bir::TypeKind::I16:
      return std::string_view{"strh"};
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return std::string_view{"str"};
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}
[[nodiscard]] const prepare::PreparedMemoryAccess* prepared_store_local_access(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return nullptr;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  return addressing != nullptr
             ? prepare::find_prepared_memory_access(
                   *addressing,
                   context.control_flow_block->block_label,
                   instruction_index)
             : nullptr;
}
[[nodiscard]] std::optional<prepare::PreparedFixedFormalStoreSourcePublication>
plan_fixed_formal_store_local_publication(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.bir_function == nullptr ||
      context.function.value_locations == nullptr ||
      store.value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto* access = prepared_store_local_access(context, instruction_index);
  const auto* source_home = prepared_value_home_for_value(context, store.value);
  const auto* destination_slot =
      access != nullptr &&
              access->address.base_kind == prepare::PreparedAddressBaseKind::FrameSlot &&
              access->address.frame_slot_id.has_value()
          ? find_frame_slot(context.function.prepared->stack_layout,
                            *access->address.frame_slot_id)
          : nullptr;
  const auto* destination_object =
      destination_slot != nullptr
          ? find_stack_object(context.function.prepared->stack_layout,
                              destination_slot->object_id)
          : nullptr;
  auto planned = prepare::plan_prepared_fixed_formal_store_source_publication(
      prepare::PreparedFormalPublicationInputs{
          .names = &context.function.prepared->names,
          .function = context.function.bir_function,
          .value_locations = context.function.value_locations,
          .value_home_lookups = context.function.value_home_lookups,
      },
      prepare::PreparedStoreSourcePublicationInputs{
          .source_value = &store.value,
          .destination_access = access,
          .source_home = source_home,
          .destination_frame_slot = destination_slot,
          .destination_stack_object = destination_object,
          .intent =
              prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
      });
  if (!planned.fixed_formal_source ||
      !prepare::prepared_store_source_publication_available(planned.store_source)) {
    return std::nullopt;
  }
  return planned;
}
[[nodiscard]] std::optional<module::MachineInstruction>
lower_fixed_formal_store_local_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      store->value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto planned =
      plan_fixed_formal_store_local_publication(context, *store, instruction_index);
  if (!planned.has_value() ||
      planned->store_source.destination_base_kind !=
          prepare::PreparedAddressBaseKind::FrameSlot ||
      !planned->store_source.destination_can_use_base_plus_offset ||
      !planned->store_source.destination_stack_offset_bytes.has_value()) {
    return std::nullopt;
  }
  const auto emitted =
      find_emitted_scalar_register(scalar_state,
                                   planned->store_source.source_value_name);
  const auto mnemonic = fixed_formal_scalar_store_mnemonic(store->value.type);
  if (!emitted.has_value() || !mnemonic.has_value() ||
      !abi::is_gp_register(emitted->reg)) {
    return std::nullopt;
  }
  const std::int64_t signed_offset =
      static_cast<std::int64_t>(
          *planned->store_source.destination_stack_offset_bytes) +
      planned->store_source.destination_byte_offset;
  if (signed_offset < 0) {
    return std::nullopt;
  }
  const auto address =
      frame_slot_address(context.function, static_cast<std::size_t>(signed_offset));
  auto store_reg = emitted->reg;
  if (const auto expected_view = scalar_register_view(store->value.type);
      expected_view.has_value()) {
    const auto resized = abi::gp_register(emitted->reg.index, *expected_view);
    if (!resized.has_value()) {
      return std::nullopt;
    }
    store_reg = *resized;
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection =
          MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = std::string{*mnemonic} + " " +
                                 std::string{abi::register_name(store_reg)} +
                                 ", " + address,
      },
  };
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
[[nodiscard]] bool block_entry_move_clobbers_current_join_publication(
    const module::BlockLoweringContext& context,
    const module::MachineInstruction& instruction) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
  }
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr || !move_record->destination_register.has_value()) {
    return false;
  }
  for (const auto& publication : collect_current_block_entry_publications(context)) {
    if (!prepare::prepared_block_entry_publication_available(publication) ||
        !publication.destination_register_name.has_value()) {
      continue;
    }
    const auto parsed = abi::parse_aarch64_register_name(
        *publication.destination_register_name);
    if (!parsed.has_value()) {
      continue;
    }
    RegisterOperand published{
        .reg = *parsed,
        .role = RegisterOperandRole::StoragePlan,
        .value_id = publication.home != nullptr
                        ? std::optional<prepare::PreparedValueId>{publication.home->value_id}
                        : std::nullopt,
        .value_name = publication.home != nullptr
                          ? publication.home->value_name
                          : c4c::kInvalidValueName,
        .expected_view = parsed->view,
    };
    if (registers_alias(published, *move_record->destination_register)) {
      return true;
    }
  }
  return false;
}
[[nodiscard]] bool prepared_value_home_reads_register_index(
    const prepare::PreparedValueHome& home,
    std::uint8_t register_index) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value()) {
    return false;
  }
  const auto parsed = abi::parse_aarch64_register_name(*home.register_name);
  return parsed.has_value() && parsed->bank == abi::RegisterBank::GeneralPurpose &&
         parsed->index == register_index;
}
[[nodiscard]] bool value_publication_may_read_register_index(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t register_index,
    unsigned depth) {
  if (depth > 64U || value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return false;
  }
  const auto* home = prepared_value_home_for_value(context, value);
  if (home != nullptr && value_has_current_block_entry_publication(context, *home)) {
    return prepared_value_home_reads_register_index(*home, register_index);
  }
  const auto prepared_producer =
      prepared_publication_source_producer_for_value(context, value);
  if (prepared_producer.has_value()) {
    const auto* producer = prepared_source_producer_instruction_for_value(
        context, *prepared_producer, value, before_instruction_index);
    if (producer != nullptr) {
      const auto producer_index = prepared_producer->instruction_index;
      if (const auto* cast = std::get_if<bir::CastInst>(producer); cast != nullptr) {
        auto operand = cast->operand;
        operand.type = cast->operand.type;
        return value_publication_may_read_register_index(
            context, operand, producer_index, register_index, depth + 1);
      }
      if (const auto* binary = std::get_if<bir::BinaryInst>(producer);
          binary != nullptr) {
        auto lhs = binary->lhs;
        lhs.type = binary->operand_type;
        auto rhs = binary->rhs;
        rhs.type = binary->operand_type;
        return value_publication_may_read_register_index(
                   context, lhs, producer_index, register_index, depth + 1) ||
               value_publication_may_read_register_index(
                   context, rhs, producer_index, register_index, depth + 1);
      }
      if (const auto* select = std::get_if<bir::SelectInst>(producer);
          select != nullptr) {
        return value_publication_may_read_register_index(
                   context,
                   select->true_value,
                   producer_index,
                   register_index,
                   depth + 1) ||
               value_publication_may_read_register_index(
                   context,
                   select->false_value,
                   producer_index,
                   register_index,
                   depth + 1);
      }
      return false;
    }
  }
  const auto producer_record = mir::find_same_block_named_producer_record(
      context.bir_block, value.name, before_instruction_index);
  const auto* producer = producer_record.inst;
  if (producer == nullptr) {
    return home != nullptr &&
           prepared_value_home_reads_register_index(*home, register_index);
  }

  const auto producer_index = producer_record ? producer_record.instruction_index
                                              : before_instruction_index;
  if (const auto* cast = std::get_if<bir::CastInst>(producer); cast != nullptr) {
    auto operand = cast->operand;
    operand.type = cast->operand.type;
    return value_publication_may_read_register_index(
        context, operand, producer_index, register_index, depth + 1);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(producer); binary != nullptr) {
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    return value_publication_may_read_register_index(
               context, lhs, producer_index, register_index, depth + 1) ||
           value_publication_may_read_register_index(
               context, rhs, producer_index, register_index, depth + 1);
  }
  if (const auto* select = std::get_if<bir::SelectInst>(producer); select != nullptr) {
    return value_publication_may_read_register_index(
               context, select->true_value, producer_index, register_index, depth + 1) ||
           value_publication_may_read_register_index(
               context, select->false_value, producer_index, register_index, depth + 1);
  }
  return false;
}
}  // namespace c4c::backend::aarch64::codegen
