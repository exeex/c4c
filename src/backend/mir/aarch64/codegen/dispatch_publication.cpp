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
#include "instruction.hpp"
#include "machine_printer.hpp"
#include "memory.hpp"
#include "operands.hpp"
#include "prepared_value_home_materialization.hpp"
#include "select_materialization.hpp"
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

[[nodiscard]] DispatchBranchFusionHooks
make_dispatch_publication_branch_fusion_hooks() {
  return DispatchBranchFusionHooks{
      .scalar_view_for_type = scalar_view_for_type,
      .emit_value_publication_to_register = emit_value_publication_to_register,
      .prepared_publication_source_producer_for_value =
          prepared_publication_source_producer_for_value,
      .prepared_source_producer_instruction =
          prepared_source_producer_instruction,
      .current_block_entry_publication_register =
          current_block_entry_publication_register,
      .find_same_block_named_producer =
          [](const module::BlockLoweringContext& context,
             std::string_view value_name,
             std::size_t before_instruction_index) -> const bir::Inst* {
        return mir::find_same_block_named_producer(
            context.bir_block, value_name, before_instruction_index);
      },
      .producer_instruction_index = producer_instruction_index,
      .prepared_value_home_for_value = prepared_value_home_for_value,
      .value_has_current_block_entry_publication =
          value_has_current_block_entry_publication,
      .emit_prepared_value_home_to_register =
          emit_prepared_value_home_to_register,
      .emit_prepared_value_home_publication_to_register =
          emit_prepared_value_home_publication_to_register,
      .fixed_slots_use_frame_pointer = fixed_slots_use_frame_pointer,
  };
}

std::optional<module::MachineInstruction>
lower_missing_conditional_branch_condition_publication(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  return lower_missing_conditional_branch_condition_publication(
      context,
      scalar_state,
      diagnostics,
      make_dispatch_publication_branch_fusion_hooks());
}
[[nodiscard]] std::optional<prepare::PreparedSameBlockScalarProducer>
prepared_same_block_publication_source_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  if (context.function.prepared_lookups != nullptr) {
    return prepare::find_prepared_same_block_scalar_producer(
        context.function.prepared->names,
        &context.function.prepared_lookups->edge_publication_source_producers,
        context.control_flow_block->block_label,
        context.bir_block,
        *value_name,
        value.type,
        before_instruction_index);
  }
  if (context.function.control_flow == nullptr) {
    return std::nullopt;
  }
  const auto source_producers =
      prepare::make_prepared_edge_publication_source_producer_lookups(
          *context.function.prepared,
          *context.function.control_flow);
  return prepare::find_prepared_same_block_scalar_producer(
      context.function.prepared->names,
      &source_producers,
      context.control_flow_block->block_label,
      context.bir_block,
      *value_name,
      value.type,
      before_instruction_index);
}
}  // namespace c4c::backend::aarch64::codegen
