#include "dispatch_publication.hpp"

#include "../../query.hpp"
#include "../../../prealloc/prepared_lookups.hpp"
#include "../abi/abi.hpp"
#include "alu.hpp"
#include "comparison.hpp"
#include "dispatch_producers.hpp"
#include "dispatch_value_materialization.hpp"
#include "frame_slot_address.hpp"
#include "memory.hpp"
#include "prepared_value_home_materialization.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
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
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr) {
    return nullptr;
  }
  return prepare::find_prepared_value_home_for_bir_value(
      context.function.prepared->names,
      context.function.value_home_lookups,
      context.function.regalloc,
      context.function.value_locations,
      value);
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
}  // namespace c4c::backend::aarch64::codegen
