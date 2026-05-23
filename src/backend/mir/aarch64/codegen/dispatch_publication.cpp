#include "dispatch_publication.hpp"

#include "dispatch_branch_fusion.hpp"
#include "dispatch_lookup.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "cast_ops.hpp"
#include "comparison.hpp"
#include "float_ops.hpp"
#include "globals.hpp"
#include "instruction.hpp"
#include "machine_printer.hpp"
#include "memory.hpp"
#include "operands.hpp"
#include "variadic.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] std::optional<std::size_t> local_slot_address_frame_offset(
    const module::BlockLoweringContext& context,
    std::string_view local_slot_name) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      local_slot_name.empty()) {
    return std::nullopt;
  }
  const prepare::PreparedFrameSlot* selected_slot = nullptr;
  for (const auto& object : context.function.prepared->stack_layout.objects) {
    if (object.function_name != context.function.control_flow->function_name ||
        object.source_kind != "local_slot") {
      continue;
    }
    const auto object_name =
        prepare::prepared_stack_object_name(context.function.prepared->names, object);
    if (object_name != local_slot_name) {
      continue;
    }
    for (const auto& slot : context.function.prepared->stack_layout.frame_slots) {
      if (slot.object_id != object.object_id) {
        continue;
      }
      if (selected_slot == nullptr ||
          slot.offset_bytes < selected_slot->offset_bytes) {
        selected_slot = &slot;
      }
    }
  }
  return selected_slot != nullptr
             ? std::optional<std::size_t>{selected_slot->offset_bytes}
             : std::nullopt;
}
[[nodiscard]] std::optional<std::size_t> local_aggregate_address_frame_offset(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name) {
  if (context.function.prepared == nullptr ||
      value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto source_name =
      prepare::prepared_value_name(context.function.prepared->names, value_name);
  if (source_name.empty()) {
    return std::nullopt;
  }
  const std::string first_lane_name = std::string{source_name} + ".0";
  return local_slot_address_frame_offset(context, first_lane_name);
}
[[nodiscard]] bool emit_local_slot_address_publication_to_register(
    const module::BlockLoweringContext& context,
    const bir::BinaryInst& binary,
    std::uint8_t target_index,
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
          ? local_slot_address_frame_offset(
                context,
                prepare::prepared_value_name(context.function.prepared->names, *lhs_name))
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
[[nodiscard]] bool register_operands_share_physical_register(
    const RegisterOperand& lhs,
    const RegisterOperand& rhs) {
  return lhs.reg.bank == rhs.reg.bank && lhs.reg.index == rhs.reg.index;
}
[[nodiscard]] bool store_local_uses_pointer_value_address(
    const bir::StoreLocalInst& store) {
  return store.address.has_value() &&
         store.address->base_kind == bir::MemoryAddress::BaseKind::PointerValue;
}
[[nodiscard]] std::optional<RegisterOperand> prepared_or_emitted_store_value_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state) {
  const auto value_name = prepared_named_value_id(context, value);
  if (value_name.has_value()) {
    if (const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
        emitted.has_value()) {
      return emitted;
    }
  }
  auto prepared = make_named_prepared_result_register(context, value);
  if (prepared.has_value()) {
    prepared->occupied_register_references = {prepared->reg};
    prepared->occupied_registers = {abi::register_name(prepared->reg)};
  }
  return prepared;
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
[[nodiscard]] std::string_view local_slot_reference_name(
    const module::BlockLoweringContext& context,
    std::string_view raw_name,
    SlotNameId slot_id) {
  if (!raw_name.empty()) {
    return raw_name;
  }
  if (context.function.prepared != nullptr && slot_id != c4c::kInvalidSlotName) {
    return context.function.prepared->module.names.slot_names.spelling(slot_id);
  }
  return {};
}
[[nodiscard]] bool local_slot_reference_matches(const module::BlockLoweringContext& context,
                                                const bir::LoadLocalInst& load,
                                                const bir::StoreLocalInst& store) {
  if (load.slot_id != c4c::kInvalidSlotName &&
      store.slot_id != c4c::kInvalidSlotName) {
    if (load.slot_id == store.slot_id) {
      return true;
    }
  }
  const auto load_name =
      local_slot_reference_name(context, load.slot_name, load.slot_id);
  const auto store_name =
      local_slot_reference_name(context, store.slot_name, store.slot_id);
  return !load_name.empty() && load_name == store_name;
}
[[nodiscard]] std::string_view prepared_frame_slot_object_name(
    const module::BlockLoweringContext& context,
    prepare::PreparedFrameSlotId slot_id) {
  if (context.function.prepared == nullptr) {
    return {};
  }
  const auto* slot = find_frame_slot(context.function.prepared->stack_layout, slot_id);
  const auto* object =
      slot != nullptr
          ? find_stack_object(context.function.prepared->stack_layout, slot->object_id)
          : nullptr;
  return object != nullptr
             ? prepare::prepared_stack_object_name(context.function.prepared->names,
                                                   *object)
             : std::string_view{};
}
[[nodiscard]] std::string_view prepared_load_local_frame_object_name(
    const module::BlockLoweringContext& context,
    std::size_t load_instruction_index) {
  const auto* access = prepared_memory_access(context, load_instruction_index);
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value()) {
    return {};
  }
  return prepared_frame_slot_object_name(context, *access->address.frame_slot_id);
}
[[nodiscard]] bool value_name_has_slot_prefix(std::string_view value_name,
                                              std::string_view slot_name) {
  return !slot_name.empty() && value_name.size() > slot_name.size() &&
         value_name.substr(0, slot_name.size()) == slot_name &&
         value_name[slot_name.size()] == '.';
}
[[nodiscard]] std::optional<std::size_t> parse_trailing_dot_offset(
    std::string_view name) {
  const auto dot = name.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= name.size()) {
    return std::nullopt;
  }
  std::size_t value = 0;
  for (std::size_t index = dot + 1; index < name.size(); ++index) {
    const char ch = name[index];
    if (ch < '0' || ch > '9') {
      return std::nullopt;
    }
    value = value * 10U + static_cast<std::size_t>(ch - '0');
  }
  return value;
}
[[nodiscard]] bool store_local_targets_logical_slot(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::string_view slot_name) {
  const auto store_name =
      local_slot_reference_name(context, store.slot_name, store.slot_id);
  if (!store_name.empty() && store_name == slot_name) {
    return true;
  }
  return store.value.kind == bir::Value::Kind::Named &&
         value_name_has_slot_prefix(store.value.name, slot_name);
}
[[nodiscard]] std::optional<NarrowLocalStorePublication>
find_latest_narrow_store_for_wide_local_load(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load,
    std::size_t load_instruction_index) {
  if (context.bir_block == nullptr) {
    return std::nullopt;
  }
  const auto load_bits = integer_bit_width(load.result.type);
  if (!load_bits.has_value()) {
    return std::nullopt;
  }
  const auto load_slot_name =
      local_slot_reference_name(context, load.slot_name, load.slot_id);
  const auto load_frame_object_name =
      prepared_load_local_frame_object_name(context, load_instruction_index);
  const auto load_lane_offset =
      load.result.kind == bir::Value::Kind::Named
          ? parse_trailing_dot_offset(load.result.name)
          : std::nullopt;
  for (std::size_t index = load_instruction_index; index > 0; --index) {
    const auto* store =
        std::get_if<bir::StoreLocalInst>(&context.bir_block->insts[index - 1]);
    if (store == nullptr ||
        (!local_slot_reference_matches(context, load, *store) &&
         !store_local_targets_logical_slot(context, *store, load_slot_name) &&
         !store_local_targets_logical_slot(context, *store, load_frame_object_name) &&
         !(load_lane_offset.has_value() &&
           store->value.kind == bir::Value::Kind::Named &&
           parse_trailing_dot_offset(store->value.name) == load_lane_offset))) {
      continue;
    }
    const auto store_bits = integer_bit_width(store->value.type);
    if (!store_bits.has_value() || *store_bits != 8U || *store_bits >= *load_bits) {
      return std::nullopt;
    }
    return NarrowLocalStorePublication{
        .stored_value = store->value,
        .instruction_index = index - 1,
    };
  }
  return std::nullopt;
}
[[nodiscard]] const prepare::PreparedValueHome* prepared_value_home_for_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.value_locations == nullptr) {
    return nullptr;
  }
  const auto value_name = prepared_named_value_id(context, value);
  return value_name.has_value()
             ? find_value_home(context,
*value_name)
             : nullptr;
}
[[nodiscard]] bool value_has_current_block_entry_publication(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& home) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
          move.destination_kind == prepare::PreparedMoveDestinationKind::Value &&
          move.to_value_id == home.value_id) {
        return true;
      }
    }
  }
  return false;
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
  auto value_name = prepared_named_value_id(context, value);
  const prepare::PreparedValueHome* home =
      value_name.has_value()
          ? find_value_home(context,
*value_name)
          : nullptr;
  if (home == nullptr && context.function.prepared != nullptr && !value.name.empty()) {
    for (const auto& candidate : context.function.value_locations->value_homes) {
      if (candidate.value_name == c4c::kInvalidValueName) {
        continue;
      }
      const auto candidate_name =
          prepare::prepared_value_name(context.function.prepared->names,
                                       candidate.value_name);
      if (candidate_name == value.name) {
        home = &candidate;
        value_name = candidate.value_name;
        break;
      }
    }
  }
  if (home == nullptr) {
    return std::nullopt;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
          move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
          move.to_value_id != home->value_id) {
        continue;
      }
      std::optional<std::string> register_name = move.destination_register_name;
      if (!register_name.has_value() &&
          home->kind == prepare::PreparedValueHomeKind::Register) {
        register_name = home->register_name;
      }
      if (!register_name.has_value()) {
        continue;
      }
      const auto parsed = abi::parse_aarch64_register_name(*register_name);
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
  }
  return std::nullopt;
}
void record_current_block_entry_publication_registers(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
          move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register) {
        continue;
      }
      const auto* home =
          find_value_home(context,
move.to_value_id);
      if (home == nullptr) {
        continue;
      }
      std::optional<std::string> register_name = move.destination_register_name;
      if (!register_name.has_value() &&
          home->kind == prepare::PreparedValueHomeKind::Register) {
        register_name = home->register_name;
      }
      if (!register_name.has_value()) {
        continue;
      }
      const auto parsed = abi::parse_aarch64_register_name(*register_name);
      if (!parsed.has_value() ||
          parsed->bank != abi::RegisterBank::GeneralPurpose) {
        continue;
      }
      record_emitted_scalar_register(
          scalar_state,
          home->value_name,
          RegisterOperand{
              .reg = *parsed,
              .role = RegisterOperandRole::StoragePlan,
              .value_id = home->value_id,
              .value_name = home->value_name,
              .expected_view = parsed->view,
          });
    }
  }
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
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
          move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register) {
        continue;
      }
      const auto* home =
          find_value_home(context,
move.to_value_id);
      std::optional<std::string> register_name = move.destination_register_name;
      if (!register_name.has_value() &&
          home != nullptr &&
          home->kind == prepare::PreparedValueHomeKind::Register) {
        register_name = home->register_name;
      }
      if (!register_name.has_value()) {
        continue;
      }
      const auto parsed = abi::parse_aarch64_register_name(*register_name);
      if (!parsed.has_value()) {
        continue;
      }
      RegisterOperand published{
          .reg = *parsed,
          .role = RegisterOperandRole::StoragePlan,
          .value_id = home != nullptr
                          ? std::optional<prepare::PreparedValueId>{home->value_id}
                          : std::nullopt,
          .value_name = home != nullptr ? home->value_name : c4c::kInvalidValueName,
          .expected_view = parsed->view,
      };
      if (registers_alias(published, *move_record->destination_register)) {
        return true;
      }
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
  const auto* producer =
      find_same_block_named_producer(context, value.name, before_instruction_index);
  const auto* home = prepared_value_home_for_value(context, value);
  if (producer == nullptr ||
      (home != nullptr && value_has_current_block_entry_publication(context, *home))) {
    return home != nullptr &&
           prepared_value_home_reads_register_index(*home, register_index);
  }

  const auto producer_index =
      producer_instruction_index(context, producer).value_or(before_instruction_index);
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
[[nodiscard]] std::optional<module::MachineInstruction>
lower_store_global_value_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const module::MachineInstruction& lowered_memory,
    std::unordered_set<c4c::ValueNameId>* published_stack_values,
    bool stack_homes_only) {
  const auto* store = std::get_if<bir::StoreGlobalInst>(&inst);
  if (store == nullptr || store->value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto store_value_name = prepared_named_value_id(context, store->value);
  if (store_value_name.has_value() && published_stack_values != nullptr &&
      published_stack_values->find(*store_value_name) != published_stack_values->end()) {
    return std::nullopt;
  }
  const auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&lowered_memory.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      !memory_record->value.has_value() ||
      (memory_record->value->kind != OperandKind::Register &&
       memory_record->value->kind != OperandKind::Memory)) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::vector<std::string> lines;
  auto value = store->value;
  bool emitted = false;
  bool published_stack_home = false;
  if (const auto* target_register =
          std::get_if<RegisterOperand>(&memory_record->value->payload);
      target_register != nullptr) {
    if (stack_homes_only) {
      return std::nullopt;
    }
    if (abi::is_reserved_mir_scratch(target_register->reg) || scratches.empty()) {
      return std::nullopt;
    }
    if (abi::is_gp_register(target_register->reg)) {
      emitted = emit_value_publication_to_register(context,
                                                   value,
                                                   instruction_index,
                                                   target_register->reg.index,
                                                   scratches.front().index,
                                                   lines);
    } else if (abi::is_fp_simd_register(target_register->reg)) {
      emitted = emit_fp_value_to_register(context,
                                          value,
                                          instruction_index,
                                          target_register->reg,
                                          scratches.front().index,
                                          lines);
    }
  } else if (const auto* target_memory =
                 std::get_if<MemoryOperand>(&memory_record->value->payload);
             target_memory != nullptr) {
    const auto store_mnemonic = scalar_store_mnemonic(value.type);
    const auto store_view = scalar_view_for_type(value.type);
    const auto store_register =
        store_view.has_value() && !scratches.empty()
            ? gp_register_name(scratches.front().index, *store_view)
            : std::nullopt;
    const auto stack_home = memory_address(*target_memory);
    if (target_memory->support != MemoryOperandSupportKind::Prepared ||
        target_memory->base_kind != MemoryBaseKind::FrameSlot ||
        !target_memory->byte_offset_is_prepared_snapshot ||
        !target_memory->can_use_base_plus_offset ||
        scratches.size() < 2U || !store_mnemonic.has_value() ||
        !store_register.has_value() || stack_home.empty()) {
      return std::nullopt;
    }
    emitted = emit_value_publication_to_register(context,
                                                 value,
                                                 instruction_index,
                                                 scratches.front().index,
                                                 scratches[1].index,
                                                 lines,
                                                 stack_homes_only);
    if (emitted) {
      lines.push_back(std::string{*store_mnemonic} + " " + *store_register +
                      ", " + stack_home);
      published_stack_home = true;
      if (store_value_name.has_value() && published_stack_values != nullptr) {
        published_stack_values->insert(*store_value_name);
      }
    }
  }
  if (!emitted || lines.empty()) {
    return std::nullopt;
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
      .side_effects = published_stack_home
                          ? std::vector<MachineSideEffectKind>{
                                MachineSideEffectKind::MemoryRead,
                                MachineSideEffectKind::MemoryWrite}
                          : std::vector<MachineSideEffectKind>{
                                MachineSideEffectKind::MemoryRead},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
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
[[nodiscard]] bool store_local_value_is_byval_frame_slot_load(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::size_t instruction_index) {
  if (store.value.kind != bir::Value::Kind::Named) {
    return false;
  }
  const auto* producer =
      find_same_block_named_producer(context, store.value.name, instruction_index);
  const auto* load = producer != nullptr ? std::get_if<bir::LoadLocalInst>(producer) : nullptr;
  if (load == nullptr) {
    return false;
  }
  const auto producer_index = producer_instruction_index(context, producer);
  const auto* access = producer_index.has_value()
                           ? prepared_memory_access(context, *producer_index)
                           : nullptr;
  return access != nullptr &&
         access->address.base_kind == prepare::PreparedAddressBaseKind::PointerValue &&
         access->address.pointer_value_name.has_value() &&
         access->address.can_use_base_plus_offset &&
         is_byval_formal_value_name(context, *access->address.pointer_value_name);
}
[[nodiscard]] bool store_local_value_is_wide_load_from_narrow_local_store(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::size_t instruction_index) {
  if (store.value.kind != bir::Value::Kind::Named) {
    return false;
  }
  const auto* producer =
      find_same_block_named_producer(context, store.value.name, instruction_index);
  const auto* load = producer != nullptr ? std::get_if<bir::LoadLocalInst>(producer) : nullptr;
  if (load == nullptr) {
    return false;
  }
  const auto producer_index = producer_instruction_index(context, producer);
  return producer_index.has_value() &&
         find_latest_narrow_store_for_wide_local_load(context, *load, *producer_index)
             .has_value();
}
[[nodiscard]] const bir::CastInst* store_local_value_cast_producer(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::size_t instruction_index) {
  if (store.value.kind != bir::Value::Kind::Named) {
    return nullptr;
  }
  const auto* producer =
      find_same_block_named_producer(context, store.value.name, instruction_index);
  return producer != nullptr ? std::get_if<bir::CastInst>(producer) : nullptr;
}
[[nodiscard]] bool store_local_value_has_select_producer(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::size_t instruction_index) {
  if (store.value.kind != bir::Value::Kind::Named) {
    return false;
  }
  const auto* producer =
      find_same_block_named_producer(context, store.value.name, instruction_index);
  return producer != nullptr && std::get_if<bir::SelectInst>(producer) != nullptr;
}
[[nodiscard]] bool store_local_value_has_scalar_fp_binary_producer(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::size_t instruction_index) {
  if (store.value.kind != bir::Value::Kind::Named) {
    return false;
  }
  const auto* producer =
      find_same_block_named_producer(context, store.value.name, instruction_index);
  const auto* binary =
      producer != nullptr ? std::get_if<bir::BinaryInst>(producer) : nullptr;
  return binary != nullptr && is_prepared_scalar_float_alu_operation(*binary);
}
[[nodiscard]] bool emit_scalar_conversion_cast_to_register(
    const module::BlockLoweringContext& context,
    const bir::CastInst& cast,
    std::size_t cast_instruction_index,
    const RegisterOperand& target_register,
    std::vector<std::string>& lines) {
  const auto gp_scratches = abi::reserved_mir_scratch_gp_registers();
  const auto fp_scratches = abi::reserved_mir_scratch_fp_simd_registers();
  if (gp_scratches.empty() || fp_scratches.empty()) {
    return false;
  }
  const std::uint8_t gp_scratch_index =
      abi::is_gp_register(target_register.reg) &&
              target_register.reg.index == gp_scratches.front().index
          ? gp_scratches.back().index
          : gp_scratches.front().index;
  const std::uint8_t fp_scratch_index =
      abi::is_fp_simd_register(target_register.reg) &&
              target_register.reg.index == fp_scratches.front().index
          ? fp_scratches.back().index
          : fp_scratches.front().index;
  switch (cast.opcode) {
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI: {
      if (!abi::is_gp_register(target_register.reg)) {
        return false;
      }
      const auto destination = scalar_gp_register_view(target_register.reg, cast.result.type);
      const auto fp_source_base = abi::fp_simd_register(fp_scratch_index, abi::RegisterView::D);
      if (!destination.has_value() || !fp_source_base.has_value() ||
          !emit_fp_value_to_register(context,
                                     cast.operand,
                                     cast_instruction_index,
                                     *fp_source_base,
                                     gp_scratch_index,
                                     lines)) {
        return false;
      }
      const auto fp_source = scalar_fp_register_view(*fp_source_base, cast.operand.type);
      if (!fp_source.has_value()) {
        return false;
      }
      lines.push_back(std::string{cast.opcode == bir::CastOpcode::FPToSI ? "fcvtzs "
                                                                         : "fcvtzu "} +
                      std::string{abi::register_name(*destination)} + ", " +
                      std::string{abi::register_name(*fp_source)});
      return true;
    }
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP: {
      if (!abi::is_fp_simd_register(target_register.reg)) {
        return false;
      }
      const auto destination = scalar_fp_register_view(target_register.reg, cast.result.type);
      const auto source_width = scalar_integer_width_bits(cast.operand.type);
      const auto gp_source = source_width.has_value()
                                 ? abi::gp_register(gp_scratch_index,
                                                    *source_width == 64U
                                                        ? abi::RegisterView::X
                                                        : abi::RegisterView::W)
                                 : std::nullopt;
      if (!destination.has_value() || !source_width.has_value() || !gp_source.has_value()) {
        return false;
      }
      if (cast.operand.kind == bir::Value::Kind::Immediate) {
        auto materialize = materialize_integer_constant_lines(
            *gp_source, immediate_integer_bits(cast.operand, *source_width), *source_width);
        if (materialize.empty()) {
          return false;
        }
        lines.insert(lines.end(), materialize.begin(), materialize.end());
      } else if (!emit_value_publication_to_register(context,
                                                     cast.operand,
                                                     cast_instruction_index,
                                                     gp_scratch_index,
                                                     target_register.reg.index,
                                                     lines)) {
        return false;
      }
      lines.push_back(std::string{cast.opcode == bir::CastOpcode::SIToFP ? "scvtf "
                                                                         : "ucvtf "} +
                      std::string{abi::register_name(*destination)} + ", " +
                      std::string{abi::register_name(*gp_source)});
      return true;
    }
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::FPTrunc: {
      if (!abi::is_fp_simd_register(target_register.reg)) {
        return false;
      }
      const auto destination = scalar_fp_register_view(target_register.reg, cast.result.type);
      const auto fp_source_base = abi::fp_simd_register(fp_scratch_index, abi::RegisterView::D);
      if (!destination.has_value() || !fp_source_base.has_value() ||
          !emit_fp_value_to_register(context,
                                     cast.operand,
                                     cast_instruction_index,
                                     *fp_source_base,
                                     gp_scratch_index,
                                     lines)) {
        return false;
      }
      const auto fp_source = scalar_fp_register_view(*fp_source_base, cast.operand.type);
      if (!fp_source.has_value()) {
        return false;
      }
      lines.push_back("fcvt " + std::string{abi::register_name(*destination)} + ", " +
                      std::string{abi::register_name(*fp_source)});
      return true;
    }
    case bir::CastOpcode::SExt:
    case bir::CastOpcode::ZExt:
    case bir::CastOpcode::Trunc:
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      return false;
  }
  return false;
}
[[nodiscard]] std::optional<module::MachineInstruction>
lower_store_local_value_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const module::MachineInstruction& lowered_memory,
    const BlockScalarLoweringState& scalar_state,
    const module::MachineBlock& block) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  const auto* cast_producer =
      store != nullptr ? store_local_value_cast_producer(context, *store, instruction_index)
                       : nullptr;
  if (store == nullptr ||
      (!store_local_value_is_byval_frame_slot_load(context, *store, instruction_index) &&
       !store_local_value_is_wide_load_from_narrow_local_store(
           context, *store, instruction_index) &&
       !store_local_value_has_select_producer(context, *store, instruction_index) &&
       !store_local_value_has_scalar_fp_binary_producer(
           context, *store, instruction_index) &&
       cast_producer == nullptr &&
       !select_chain_contains_direct_global_load(context, store->value, instruction_index))) {
    return std::nullopt;
  }
  const auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&lowered_memory.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      !memory_record->value.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::vector<std::string> lines;
  auto value = store->value;
  const auto* producer_inst =
      value.kind == bir::Value::Kind::Named
          ? find_same_block_named_producer(context, value.name, instruction_index)
          : nullptr;
  const auto producer_index = producer_instruction_index(context, producer_inst);
  if (producer_index.has_value() &&
      std::get_if<bir::SelectInst>(producer_inst) != nullptr &&
      !block.instructions.empty() &&
      block.instructions.back().origin.has_value() &&
      block.instructions.back().origin->instruction_index == *producer_index) {
    return std::nullopt;
  }
  bool emitted = false;
  bool published_stack_home = false;
  if (const auto* target_register =
          std::get_if<RegisterOperand>(&memory_record->value->payload);
      target_register != nullptr) {
    if (const auto value_name = prepared_named_value_id(context, value);
        value_name.has_value() &&
        store_local_value_has_scalar_fp_binary_producer(
            context, *store, instruction_index)) {
      const auto emitted_value =
          find_emitted_scalar_register(scalar_state, *value_name);
      if (emitted_value.has_value() &&
          register_operands_share_physical_register(
              *emitted_value, *target_register)) {
        return std::nullopt;
      }
    }
    if (store_local_uses_pointer_value_address(*store)) {
      const auto value_register =
          prepared_or_emitted_store_value_register(context, value, scalar_state);
      if (value_register.has_value() &&
          register_operands_share_physical_register(*value_register, *target_register)) {
        return std::nullopt;
      }
    }
    if (abi::is_reserved_mir_scratch(target_register->reg)) {
      const auto* value_home = prepared_value_home_for_value(context, value);
      const auto store_mnemonic = scalar_store_mnemonic(value.type);
      const auto store_view = scalar_view_for_type(value.type);
      const auto store_register =
          store_view.has_value()
              ? gp_register_name(target_register->reg.index, *store_view)
              : std::nullopt;
      std::optional<std::uint8_t> alternate_scratch;
      for (const auto& scratch : scratches) {
        if (scratch.index != target_register->reg.index) {
          alternate_scratch = scratch.index;
          break;
        }
      }
      if (!store_local_value_has_select_producer(context, *store, instruction_index) ||
          value_home == nullptr ||
          value_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
          !value_home->offset_bytes.has_value() ||
          !store_mnemonic.has_value() ||
          !store_register.has_value() ||
          !alternate_scratch.has_value()) {
        return std::nullopt;
      }
      emitted = emit_value_publication_to_register(context,
                                                   value,
                                                   instruction_index,
                                                   target_register->reg.index,
                                                   *alternate_scratch,
                                                   lines,
                                                   true);
      if (emitted) {
        lines.push_back(std::string{*store_mnemonic} + " " + *store_register +
                        ", " + frame_slot_address(context.function, *value_home->offset_bytes));
        published_stack_home = true;
      }
    } else if (cast_producer != nullptr) {
      emitted = emit_scalar_conversion_cast_to_register(
          context,
          *cast_producer,
          producer_instruction_index(context, producer_inst).value_or(instruction_index),
          *target_register,
          lines);
    }
    if (!emitted && abi::is_fp_simd_register(target_register->reg) &&
        store_local_value_has_scalar_fp_binary_producer(
            context, *store, instruction_index) &&
        !scratches.empty()) {
      lines.clear();
      emitted = emit_fp_value_to_register(context,
                                          value,
                                          instruction_index,
                                          target_register->reg,
                                          scratches.front().index,
                                          lines);
    }
    if (!emitted && abi::is_gp_register(target_register->reg) && !scratches.empty()) {
      lines.clear();
      emitted = emit_value_publication_to_register(context,
                                                   value,
                                                   instruction_index,
                                                   target_register->reg.index,
                                                   scratches.front().index,
                                                   lines);
    }
  } else if (const auto* target_memory =
                 std::get_if<MemoryOperand>(&memory_record->value->payload);
             target_memory != nullptr &&
             (store_local_value_has_select_producer(context, *store, instruction_index) ||
              select_chain_contains_direct_global_load(context, value, instruction_index))) {
    const auto store_mnemonic = scalar_store_mnemonic(value.type);
    const auto store_view = scalar_view_for_type(value.type);
    const auto store_register =
        store_view.has_value() && !scratches.empty()
            ? gp_register_name(scratches.front().index, *store_view)
            : std::nullopt;
    const auto stack_home = memory_address(*target_memory);
    if (target_memory->support != MemoryOperandSupportKind::Prepared ||
        target_memory->base_kind != MemoryBaseKind::FrameSlot ||
        !target_memory->byte_offset_is_prepared_snapshot ||
        !target_memory->can_use_base_plus_offset ||
        scratches.size() < 2U ||
        !store_mnemonic.has_value() ||
        !store_register.has_value() ||
        stack_home.empty()) {
      return std::nullopt;
    }
    emitted = emit_value_publication_to_register(context,
                                                 value,
                                                 instruction_index,
                                                 scratches.front().index,
                                                 scratches[1].index,
                                                 lines);
    if (emitted) {
      lines.push_back(std::string{*store_mnemonic} + " " + *store_register +
                      ", " + stack_home);
      published_stack_home = true;
    }
  }
  if (!emitted || lines.empty()) {
    return std::nullopt;
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
      .side_effects = published_stack_home
                          ? std::vector<MachineSideEffectKind>{
                                MachineSideEffectKind::MemoryRead,
                                MachineSideEffectKind::MemoryWrite}
                          : std::vector<MachineSideEffectKind>{
                                MachineSideEffectKind::MemoryRead},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
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
[[nodiscard]] std::optional<module::MachineInstruction>
lower_stack_homed_pointer_store_writeback(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      !store->address.has_value() ||
      store->address->base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
      context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto* access = prepared_memory_access(context, instruction_index);
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.byte_offset < 0) {
    return std::nullopt;
  }
  const auto* pointer_home =
      find_value_home(context,
*access->address.pointer_value_name);
  if (pointer_home == nullptr ||
      pointer_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !pointer_home->offset_bytes.has_value()) {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  const auto value_view = scalar_view_for_type(store->value.type);
  const auto store_mnemonic = scalar_store_mnemonic(store->value.type);
  if (scratches.size() < 2U || !value_view.has_value() ||
      !store_mnemonic.has_value()) {
    return std::nullopt;
  }
  const auto value_register = gp_register_name(scratches[0].index, *value_view);
  const auto address_register_ref =
      abi::gp_register(scratches[1].index, abi::RegisterView::X);
  const auto address_register = gp_register_name(scratches[1].index, abi::RegisterView::X);
  if (!value_register.has_value() || !address_register_ref.has_value() ||
      !address_register.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  std::string stored_register = *value_register;
  if (auto prepared_store_value =
          make_named_prepared_result_register(context, store->value);
      prepared_store_value.has_value() &&
      !register_operands_share_physical_register(
          *prepared_store_value,
          RegisterOperand{.reg = *address_register_ref})) {
    const auto source_register = std::string{abi::register_name(prepared_store_value->reg)};
    if (source_register != stored_register) {
      lines.push_back("mov " + stored_register + ", " + source_register);
    }
  } else {
    if (!emit_value_publication_to_register(context,
                                            store->value,
                                            instruction_index,
                                            scratches[0].index,
                                            scratches[1].index,
                                            lines,
                                            true)) {
      return std::nullopt;
    }
  }
  lines.push_back("ldr " + *address_register + ", " +
                  frame_slot_address(context.function, *pointer_home->offset_bytes));
  lines.push_back(std::string{*store_mnemonic} + " " + stored_register +
                  ", " + register_indirect_address(
                              *address_register,
                              static_cast<std::size_t>(access->address.byte_offset)));

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
      .side_effects =
          {MachineSideEffectKind::MemoryRead, MachineSideEffectKind::MemoryWrite},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
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
[[nodiscard]] std::optional<std::string> prepared_global_symbol_from_value_name(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name) {
  if (context.function.prepared == nullptr || value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const std::string_view spelling =
      context.function.prepared->names.value_names.spelling(value_name);
  if (spelling.size() <= 1 || spelling.front() != '@') {
    if (context.function.prepared->names.link_names.find(spelling) != c4c::kInvalidLinkName) {
      return std::string{spelling};
    }
    return std::nullopt;
  }
  return std::string{spelling.substr(1)};
}
[[nodiscard]] bool emit_global_symbol_address_to_register(
    std::string_view symbol,
    std::int64_t delta,
    std::uint8_t target_index,
    std::vector<std::string>& lines) {
  const auto target = gp_register_name(target_index, abi::RegisterView::X);
  if (!target.has_value() || symbol.empty()) {
    return false;
  }
  if (delta >= 0) {
    const auto reloc = relocation_operand(std::string{symbol}, static_cast<std::size_t>(delta));
    lines.push_back("adrp " + *target + ", " + reloc);
    lines.push_back("add " + *target + ", " + *target + ", :lo12:" + reloc);
    return true;
  }
  lines.push_back("adrp " + *target + ", " + std::string{symbol});
  lines.push_back("add " + *target + ", " + *target + ", :lo12:" + std::string{symbol});
  lines.push_back("sub " + *target + ", " + *target + ", #" +
                  std::to_string(-delta));
  return true;
}
[[nodiscard]] bool emit_pointer_base_plus_offset_to_register(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& value_home,
    std::size_t instruction_index,
    std::uint8_t target_index,
    std::vector<std::string>& lines) {
  if (value_home.kind != prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      !value_home.pointer_base_value_name.has_value()) {
    return false;
  }
  const auto target = gp_register_name(target_index, abi::RegisterView::X);
  if (!target.has_value()) {
    return false;
  }

  const auto delta = value_home.pointer_byte_delta.value_or(0);
  if (auto symbol =
          prepared_global_symbol_from_value_name(context, *value_home.pointer_base_value_name)) {
    return emit_global_symbol_address_to_register(
        *symbol, delta, target_index, lines);
  }

  const auto base_value_spelling =
      context.function.prepared != nullptr
          ? context.function.prepared->names.value_names.spelling(
                *value_home.pointer_base_value_name)
          : std::string_view{};
  const auto* producer =
      find_same_block_named_producer(context, base_value_spelling, instruction_index);
  if (const auto* load_local =
          producer != nullptr ? std::get_if<bir::LoadLocalInst>(producer) : nullptr;
      load_local != nullptr && load_local->result.type == bir::TypeKind::Ptr) {
    const auto producer_index = producer_instruction_index(context, producer);
    const auto offset = producer_index.has_value()
                            ? prepared_local_load_offset(context, *producer_index)
                            : std::nullopt;
    if (offset.has_value()) {
      lines.push_back("ldr " + *target + ", " + frame_slot_address(context.function, *offset));
      if (delta > 0) {
        lines.push_back("add " + *target + ", " + *target + ", #" +
                        std::to_string(delta));
      } else if (delta < 0) {
        lines.push_back("sub " + *target + ", " + *target + ", #" +
                        std::to_string(-delta));
      }
      return true;
    }
  }

  if (context.function.value_locations == nullptr) {
    return false;
  }
  const auto* base_home =
      find_value_home(context,
*value_home.pointer_base_value_name);
  if (base_home == nullptr) {
    return false;
  }
  if (base_home->kind == prepare::PreparedValueHomeKind::Register &&
      base_home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*base_home->register_name);
    if (!parsed.has_value() || !abi::is_gp_register(*parsed)) {
      return false;
    }
    const auto source = gp_register_name(parsed->index, abi::RegisterView::X);
    if (!source.has_value()) {
      return false;
    }
    if (delta == 0) {
      if (*source != *target) {
        lines.push_back("mov " + *target + ", " + *source);
      }
    } else if (delta > 0) {
      lines.push_back("add " + *target + ", " + *source + ", #" +
                      std::to_string(delta));
    } else {
      lines.push_back("sub " + *target + ", " + *source + ", #" +
                      std::to_string(-delta));
    }
    return true;
  }
  if (base_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      base_home->offset_bytes.has_value()) {
    lines.push_back("ldr " + *target + ", " + frame_slot_address(context.function, *base_home->offset_bytes));
    if (delta > 0) {
      lines.push_back("add " + *target + ", " + *target + ", #" +
                      std::to_string(delta));
    } else if (delta < 0) {
      lines.push_back("sub " + *target + ", " + *target + ", #" +
                      std::to_string(-delta));
    }
    return true;
  }
  return false;
}
[[nodiscard]] std::optional<std::size_t> store_local_frame_slot_offset(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr) {
    return std::nullopt;
  }
  const auto* access = prepared_memory_access(context, instruction_index);
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.byte_offset < 0) {
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
[[nodiscard]] std::optional<module::MachineInstruction>
lower_pointer_base_plus_offset_store_local_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      store->value.kind != bir::Value::Kind::Named ||
      store->value.type != bir::TypeKind::Ptr) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, store->value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto destination_offset = store_local_frame_slot_offset(context, instruction_index);
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (!destination_offset.has_value() || scratches.empty()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  const auto address_register = gp_register_name(scratches.front().index, abi::RegisterView::X);
  if (!address_register.has_value()) {
    return std::nullopt;
  }

  bool emitted = false;
  if (auto symbol = prepared_global_symbol_from_value_name(context, *value_name)) {
    emitted =
        emit_global_symbol_address_to_register(*symbol, 0, scratches.front().index, lines);
  } else if (context.function.value_locations != nullptr) {
    const auto* value_home =
        find_value_home(context,
*value_name);
    emitted = value_home != nullptr &&
              value_home->kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset &&
              emit_pointer_base_plus_offset_to_register(
                  context, *value_home, instruction_index, scratches.front().index, lines);
  }
  if (!emitted) {
    return std::nullopt;
  }
  lines.push_back("str " + *address_register + ", " + frame_slot_address(context.function, *destination_offset));

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
      .side_effects = {MachineSideEffectKind::MemoryWrite},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
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
[[nodiscard]] bool is_store_global_select_snapshot_run_instruction(const bir::Inst& inst) {
  return std::get_if<bir::SelectInst>(&inst) != nullptr ||
         std::get_if<bir::LoadGlobalInst>(&inst) != nullptr ||
         std::get_if<bir::BinaryInst>(&inst) != nullptr ||
         std::get_if<bir::CastInst>(&inst) != nullptr ||
         std::get_if<bir::StoreGlobalInst>(&inst) != nullptr;
}
void lower_pending_store_global_stack_value_publications(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::unordered_set<c4c::ValueNameId>& published_stack_values,
    module::MachineBlock& block) {
  if (context.bir_block == nullptr) {
    return;
  }
  for (std::size_t index = instruction_index; index < context.bir_block->insts.size();
       ++index) {
    const auto& candidate = context.bir_block->insts[index];
    if (!is_store_global_select_snapshot_run_instruction(candidate)) {
      break;
    }
    if (std::get_if<bir::StoreGlobalInst>(&candidate) == nullptr) {
      continue;
    }
    module::ModuleLoweringDiagnostics ignored_diagnostics;
    auto lowered_memory =
        lower_memory_instruction(context, candidate, index, ignored_diagnostics);
    if (!lowered_memory.handled || !lowered_memory.instruction.has_value()) {
      continue;
    }
    if (auto publication =
            lower_store_global_value_publication(context,
                                                 candidate,
                                                 index,
                                                 *lowered_memory.instruction,
                                                 &published_stack_values,
                                                 true)) {
      block.instructions.push_back(std::move(*publication));
    }
  }
}
[[nodiscard]] std::optional<bir::Value> instruction_result_value(
    const bir::Inst& inst) {
  const auto* result = instruction_result_value_ref(inst);
  if (result == nullptr) {
    return std::nullopt;
  }
  return *result;
}
[[nodiscard]] const bir::Value* instruction_result_value_ref(const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> const bir::Value* {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst>) {
          return &typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::CastInst>) {
          return &typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          return &typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::LoadLocalInst>) {
          return &typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::LoadGlobalInst>) {
          return &typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return typed_inst.result.has_value() ? &*typed_inst.result : nullptr;
        }
        return nullptr;
      },
      inst);
}

}  // namespace c4c::backend::aarch64::codegen
