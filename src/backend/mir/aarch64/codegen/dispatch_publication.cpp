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
