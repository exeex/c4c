#include "dispatch_value_materialization.hpp"

#include "dispatch.hpp"
#include "dispatch_edge_copies.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "comparison.hpp"
#include "constant_materialization.hpp"
#include "dispatch_producers.hpp"
#include "dispatch_publication.hpp"
#include "fp_value_materialization.hpp"
#include "float_ops.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "operands.hpp"
#include "prepared_value_home_materialization.hpp"
#include "variadic.hpp"
#include "../../../prealloc/prepared_lookups.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
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

[[nodiscard]] std::optional<prepare::PreparedSameBlockScalarProducer>
prepared_same_block_scalar_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr || context.control_flow_block == nullptr ||
      context.function.prepared == nullptr ||
      context.function.prepared_lookups == nullptr ||
      context.function.control_flow == nullptr ||
      value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  return prepare::find_prepared_same_block_scalar_producer(
      context.function.prepared->names,
      &context.function.prepared_lookups->edge_publication_source_producers,
      context.control_flow_block->block_label,
      context.bir_block,
      *value_name,
      value.type,
      before_instruction_index);
}

[[nodiscard]] prepare::PreparedScalarSelectChainMaterialization
prepared_scalar_select_chain_materialization(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.prepared_lookups == nullptr ||
      context.control_flow_block == nullptr) {
    return prepare::PreparedScalarSelectChainMaterialization{};
  }
  return prepare::find_prepared_scalar_select_chain_materialization(
      context.function.prepared->names,
      &context.function.prepared_lookups->edge_publication_source_producers,
      context.control_flow_block->block_label,
      context.bir_block,
      value,
      before_instruction_index);
}

[[nodiscard]] std::optional<std::int64_t>
prepared_same_block_integer_constant(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return value.immediate;
  }
  if (context.bir_block == nullptr || context.control_flow_block == nullptr ||
      context.function.prepared == nullptr ||
      context.function.prepared_lookups == nullptr ||
      value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  return prepare::evaluate_prepared_same_block_integer_constant(
      context.function.prepared->names,
      &context.function.prepared_lookups->edge_publication_source_producers,
      context.control_flow_block->block_label,
      context.bir_block,
      value,
      before_instruction_index);
}

[[nodiscard]] bool emit_prepared_global_load_to_register(
    const module::BlockLoweringContext& context,
    const bir::LoadGlobalInst& load_global,
    const prepare::PreparedMemoryAccess& access,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines) {
  if (context.function.prepared == nullptr) {
    return false;
  }
  const auto symbol_label =
      prepare::prepared_link_name(context.function.prepared->names,
                                  *access.address.symbol_name);
  const auto mnemonic = scalar_load_mnemonic(load_global.result.type);
  const auto load_view = scalar_view_for_type(load_global.result.type);
  const auto load_target =
      load_view.has_value() ? gp_register_name(target_index, *load_view) : std::nullopt;
  const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
  if (symbol_label.empty() || !mnemonic.has_value() || !load_target.has_value() ||
      !address.has_value()) {
    return false;
  }

  const auto policy =
      prepared_load_global_address_policy(context, load_global, access);
  if (!policy.has_value()) {
    return false;
  }

  switch (*policy) {
    case bir::GlobalAddressMaterializationPolicy::GotRequired:
      lines.push_back("adrp " + *address + ", :got:" + std::string{symbol_label});
      lines.push_back("ldr " + *address + ", [" + *address + ", :got_lo12:" +
                      std::string{symbol_label} + "]");
      lines.push_back(std::string{*mnemonic} + " " + *load_target + ", " +
                      register_indirect_address(*address, access.address.byte_offset));
      return true;
    case bir::GlobalAddressMaterializationPolicy::Direct: {
      const auto symbol = relocation_operand(symbol_label, access.address.byte_offset);
      lines.push_back("adrp " + *address + ", " + symbol);
      lines.push_back("add " + *address + ", " + *address + ", :lo12:" + symbol);
      lines.push_back(std::string{*mnemonic} + " " + *load_target + ", [" + *address +
                      "]");
      return true;
    }
    case bir::GlobalAddressMaterializationPolicy::Unspecified:
      return false;
  }
  return false;
}

}  // namespace

[[nodiscard]] std::optional<unsigned> value_power_of_two_shift(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (value.kind == bir::Value::Kind::Immediate) {
    if (value.immediate < 0) {
      return std::nullopt;
    }
    return power_of_two_shift(static_cast<std::uint64_t>(value.immediate));
  }
  const auto constant =
      prepared_same_block_integer_constant(context, value, before_instruction_index);
  if (!constant.has_value() || *constant < 0) {
    return std::nullopt;
  }
  return power_of_two_shift(static_cast<std::uint64_t>(*constant));
}

[[nodiscard]] bool value_publication_may_write_scratch_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth = 0) {
  if (depth > 64U || value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return false;
  }
  if (prepared_same_block_integer_constant(
          context, value, before_instruction_index).has_value()) {
    return false;
  }
  if (const auto value_view = scalar_view_for_type(value.type);
      value_view.has_value()) {
    if (current_block_entry_publication_register(context, value, *value_view)) {
      return false;
    }
  }

  const auto producer_context =
      prepared_same_block_scalar_producer(context, value, before_instruction_index);
  if (!producer_context.has_value() || producer_context->instruction == nullptr) {
    return false;
  }
  const auto* producer = producer_context->instruction;
  const auto producer_index = producer_context->instruction_index;

  if (const auto* cast = std::get_if<bir::CastInst>(producer); cast != nullptr) {
    auto operand = cast->operand;
    operand.type = cast->operand.type;
    return value_publication_may_write_scratch_register(
        context, operand, producer_index, depth + 1);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(producer); binary != nullptr) {
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    if (binary->opcode == bir::BinaryOpcode::Mul &&
        value_power_of_two_shift(context, rhs, producer_index).has_value()) {
      return value_publication_may_write_scratch_register(
          context, lhs, producer_index, depth + 1);
    }
    return true;
  }
  if (std::get_if<bir::LoadGlobalInst>(producer) != nullptr ||
      std::get_if<bir::LoadLocalInst>(producer) != nullptr ||
      std::get_if<bir::SelectInst>(producer) != nullptr) {
    return true;
  }
  return false;
}

[[nodiscard]] bool emit_value_publication_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines,
    bool reload_current_memory_loads) {
  const auto target_view = scalar_view_for_type(value.type);
  if (!target_view.has_value()) {
    return false;
  }
  const auto target = gp_register_name(target_index, *target_view);
  if (!target.has_value()) {
    return false;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    lines.push_back("mov " + *target + ", #" + std::to_string(value.immediate));
    return true;
  }
  if (value.kind != bir::Value::Kind::Named) {
    return false;
  }
  if (const auto constant =
          prepared_same_block_integer_constant(context, value, before_instruction_index);
      constant.has_value()) {
    lines.push_back("mov " + *target + ", #" + std::to_string(*constant));
    return true;
  }
  if (auto published =
          current_block_entry_publication_register(context, value, *target_view)) {
    const auto source = abi::gp_register(published->reg.index, *target_view);
    if (!source.has_value()) {
      return false;
    }
    const auto source_name = std::string{abi::register_name(*source)};
    if (source_name != *target) {
      lines.push_back("mov " + *target + ", " + source_name);
    }
    return true;
  }
  const auto producer_context =
      prepared_same_block_scalar_producer(context, value, before_instruction_index);
  const auto* producer =
      producer_context.has_value() ? producer_context->instruction : nullptr;
  if (producer != nullptr &&
      is_current_block_join_parallel_copy_source(context, *producer)) {
    const auto* home = prepared_value_home_for_value(context, value);
    if (home != nullptr) {
      return emit_prepared_value_home_publication_to_register(context,
                                                             value,
                                                             *home,
                                                             target_index,
                                                             lines);
    }
  }
  if (producer == nullptr) {
    const auto* home = prepared_value_home_for_value(context, value);
    if (home != nullptr) {
      return emit_prepared_value_home_publication_to_register(context,
                                                             value,
                                                             *home,
                                                             target_index,
                                                             lines);
    }
    return false;
  }
  if (const auto* home = prepared_value_home_for_value(context, value);
      home != nullptr && value_has_current_block_entry_publication(context, *home)) {
    return emit_prepared_value_home_publication_to_register(context,
                                                           value,
                                                           *home,
                                                           target_index,
                                                           lines);
  }

  if (const auto* load_local = std::get_if<bir::LoadLocalInst>(producer);
      load_local != nullptr) {
    const auto index = producer_context->instruction_index;
    if (const auto* home = prepared_value_home_for_value(context, value);
        !reload_current_memory_loads && home != nullptr) {
      return emit_prepared_value_home_to_register(context.function.prepared != nullptr
                                                     ? &context.function.prepared->stack_layout
                                                     : nullptr,
                                                 *home,
                                                 value.type,
                                                 target_index,
                                                 lines,
                                                 fixed_slots_use_frame_pointer(context.function));
    }
    if (emit_prepared_va_list_field_load_to_register(
            context, *load_local, target_index, lines)) {
      return true;
    }
    if (emit_prepared_global_symbol_load_to_register(context,
                                                     index,
                                                     load_local->result.type,
                                                     target_index,
                                                     scratch_index,
                                                     lines)) {
      return true;
    }
    const auto* load_access = prepared_memory_access(context, index);
    if (!prepared_memory_access_matches_instruction(context, load_access, *producer)) {
      return false;
    }
    const auto* addressing =
        context.function.prepared != nullptr && context.function.control_flow != nullptr
            ? prepare::find_prepared_addressing(
                  *context.function.prepared,
                  context.function.control_flow->function_name)
            : nullptr;
    const auto narrow_store =
        context.function.prepared != nullptr && context.control_flow_block != nullptr
            ? prepare::find_prepared_recovered_narrow_store_source_for_wide_local_load(
                  context.function.prepared->names,
                  context.function.prepared->module.names,
                  context.function.prepared->stack_layout,
                  addressing,
                  context.control_flow_block->block_label,
                  context.bir_block,
                  *load_local,
                  index)
            : std::nullopt;
    if (narrow_store.has_value() &&
        emit_value_publication_to_register(context,
                                           narrow_store->stored_value,
                                           narrow_store->instruction_index,
                                           target_index,
                                           scratch_index,
                                           lines,
                                           reload_current_memory_loads)) {
      return true;
    }
    const auto offset = prepared_local_load_offset(context, index);
    if (offset.has_value()) {
      const auto mnemonic = scalar_load_mnemonic(load_local->result.type);
      const auto load_view = scalar_view_for_type(load_local->result.type);
      const auto load_target =
          load_view.has_value() ? gp_register_name(target_index, *load_view) : std::nullopt;
      if (!mnemonic.has_value() || !load_target.has_value()) {
        return false;
      }
      lines.push_back(std::string{*mnemonic} + " " + *load_target + ", " +
                      frame_slot_address(context.function, *offset));
      return true;
    }
    return emit_prepared_pointer_value_load_to_register(
        context, *load_local, index, target_index, scratch_index, lines);
  }

  if (std::get_if<bir::LoadGlobalInst>(producer) != nullptr) {
    if (const auto* home = prepared_value_home_for_value(context, value);
        !reload_current_memory_loads && home != nullptr &&
        home->kind == prepare::PreparedValueHomeKind::StackSlot &&
        home->offset_bytes.has_value()) {
      return emit_prepared_value_home_to_register(context.function.prepared != nullptr
                                                     ? &context.function.prepared->stack_layout
                                                     : nullptr,
                                                 *home,
                                                 value.type,
                                                 target_index,
                                                 lines,
                                                 fixed_slots_use_frame_pointer(context.function));
    }
    const auto* addressing =
        context.function.prepared != nullptr && context.function.control_flow != nullptr
            ? prepare::find_prepared_addressing(
                  *context.function.prepared,
                  context.function.control_flow->function_name)
            : nullptr;
    const auto prepared_access =
        context.function.prepared != nullptr
            ? prepare::find_prepared_same_block_global_load_access(
                  context.function.prepared->names,
                  addressing,
                  *producer_context)
            : std::nullopt;
    return prepared_access.has_value() &&
           prepared_access->load_global != nullptr &&
           prepared_access->access != nullptr &&
           emit_prepared_global_load_to_register(context,
                                                 *prepared_access->load_global,
                                                 *prepared_access->access,
                                                 target_index,
                                                 scratch_index,
                                                 lines);
  }

  if (const auto* cast = std::get_if<bir::CastInst>(producer); cast != nullptr) {
    const auto cast_index = producer_context->instruction_index;
    if (cast->opcode == bir::CastOpcode::SExt) {
      bir::Value source = cast->operand;
      source.type = cast->operand.type;
      if (!emit_value_publication_to_register(
              context,
              source,
              cast_index,
              target_index,
              scratch_index,
              lines,
              reload_current_memory_loads)) {
        return false;
      }
      const auto source_bits = integer_bit_width(cast->operand.type);
      const auto result_bits = integer_bit_width(cast->result.type);
      const auto source_name = gp_register_name(target_index, abi::RegisterView::W);
      const auto target_name = gp_register_name(target_index, *target_view);
      if (!source_bits.has_value() || !result_bits.has_value() ||
          !source_name.has_value() || !target_name.has_value() ||
          *source_bits >= *result_bits) {
        return false;
      }
      if (*source_bits == 8U) {
        lines.push_back("sxtb " + *target_name + ", " + *source_name);
      } else if (*source_bits == 16U) {
        lines.push_back("sxth " + *target_name + ", " + *source_name);
      } else if (*source_bits == 32U && *result_bits == 64U) {
        lines.push_back("sxtw " + *target_name + ", " + *source_name);
      } else {
        return false;
      }
      return true;
    }
    if (cast->opcode == bir::CastOpcode::ZExt) {
      bir::Value source = cast->operand;
      source.type = cast->operand.type;
      if (!emit_value_publication_to_register(
              context,
              source,
              cast_index,
              target_index,
              scratch_index,
              lines,
              reload_current_memory_loads)) {
        return false;
      }
      const auto source_bits = integer_bit_width(cast->operand.type);
      const auto result_bits = integer_bit_width(cast->result.type);
      if (!source_bits.has_value() || !result_bits.has_value()) {
        return false;
      }
      if (*source_bits < 32U || *source_bits < *result_bits) {
        const auto widened = gp_register_name(target_index, abi::RegisterView::X);
        if (!widened.has_value()) {
          return false;
        }
        if (*source_bits < 32U) {
          lines.push_back("ubfx " + *widened + ", " + *widened +
                          ", #0, #" + std::to_string(*source_bits));
        }
      }
      return true;
    }
    if (cast->opcode == bir::CastOpcode::Trunc) {
      bir::Value source = cast->operand;
      source.type = cast->operand.type;
      return emit_value_publication_to_register(
          context,
          source,
          cast_index,
          target_index,
          scratch_index,
          lines,
          reload_current_memory_loads);
    }
    return false;
  }

  if (std::get_if<bir::SelectInst>(producer) != nullptr) {
    const auto select_chain_materialization =
        prepared_scalar_select_chain_materialization(
            context, value, before_instruction_index);
    if (!select_chain_materialization.available ||
        !select_chain_materialization.root_instruction_index.has_value()) {
      return false;
    }
    const auto* direct_global_dependency =
        select_chain_materialization.direct_global_dependency.contains_direct_global_load
            ? &select_chain_materialization.direct_global_dependency
            : nullptr;
    std::size_t label_index = 0;
    std::vector<std::string_view> active_values;
    return emit_select_chain_value_to_register(context,
                                               value,
                                               before_instruction_index,
                                               target_index,
                                               scratch_index,
                                               *select_chain_materialization
                                                    .root_instruction_index,
                                               select_chain_materialization
                                                    .root_value_name,
                                               lines,
                                               label_index,
                                               active_values,
                                               reload_current_memory_loads,
                                               direct_global_dependency);
  }

  const auto* binary = std::get_if<bir::BinaryInst>(producer);
  if (binary == nullptr) {
    return false;
  }
  const auto binary_index = producer_context->instruction_index;

  if (const auto condition = branch_condition_suffix(binary->opcode);
      condition.has_value()) {
    if (binary->operand_type == bir::TypeKind::F32 ||
        binary->operand_type == bir::TypeKind::F64) {
      const auto result_name =
          gp_register_name(target_index, scalar_view_for_type(binary->result.type)
                                             .value_or(abi::RegisterView::W));
      const auto fp_scratches = abi::reserved_mir_scratch_fp_simd_registers();
      if (!result_name.has_value() || fp_scratches.size() < 2U) {
        return false;
      }
      const auto lhs_fp = scalar_fp_register_view(fp_scratches[0], binary->operand_type);
      const auto rhs_fp = scalar_fp_register_view(fp_scratches[1], binary->operand_type);
      if (!lhs_fp.has_value() || !rhs_fp.has_value()) {
        return false;
      }
      auto lhs = binary->lhs;
      lhs.type = binary->operand_type;
      auto rhs = binary->rhs;
      rhs.type = binary->operand_type;
      if (!emit_fp_value_to_register(context,
                                     lhs,
                                     binary_index,
                                     *lhs_fp,
                                     scratch_index,
                                     lines) ||
          !emit_fp_value_to_register(context,
                                     rhs,
                                     binary_index,
                                     *rhs_fp,
                                     target_index,
                                     lines)) {
        return false;
      }
      lines.push_back("fcmp " + std::string{abi::register_name(*lhs_fp)} + ", " +
                      std::string{abi::register_name(*rhs_fp)});
      lines.push_back("cset " + *result_name + ", " + std::string{*condition});
      return true;
    }
    const auto operand_view = scalar_view_for_type(binary->operand_type);
    const auto lhs_name =
        operand_view.has_value() ? gp_register_name(target_index, *operand_view)
                                 : std::nullopt;
    const auto result_name =
        gp_register_name(target_index, scalar_view_for_type(binary->result.type)
                                           .value_or(abi::RegisterView::W));
    if (!operand_view.has_value() || !lhs_name.has_value() ||
        !result_name.has_value()) {
      return false;
    }
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    if (!emit_value_publication_to_register(
            context,
            lhs,
            binary_index,
            target_index,
            scratch_index,
            lines,
            reload_current_memory_loads)) {
      return false;
    }
    std::optional<std::string> rhs_name;
    if (binary->rhs.kind == bir::Value::Kind::Immediate &&
        is_cmp_immediate_encodable(binary->rhs.immediate)) {
      rhs_name = "#" + std::to_string(binary->rhs.immediate);
    } else {
      rhs_name = gp_register_name(scratch_index, *operand_view);
      auto rhs = binary->rhs;
      rhs.type = binary->operand_type;
      const std::uint8_t nested_scratch_index = scratch_index == 9 ? 10 : 9;
      if (!rhs_name.has_value() ||
          !emit_value_publication_to_register(context,
                                              rhs,
                                              binary_index,
                                              scratch_index,
                                              nested_scratch_index,
                                              lines,
                                              reload_current_memory_loads)) {
        return false;
      }
    }
    lines.push_back("cmp " + *lhs_name + ", " + *rhs_name);
    lines.push_back("cset " + *result_name + ", " + std::string{*condition});
    return true;
  }

  if (binary->opcode == bir::BinaryOpcode::And ||
      binary->opcode == bir::BinaryOpcode::Or ||
      binary->opcode == bir::BinaryOpcode::Xor) {
    const auto operand_view = scalar_view_for_type(binary->operand_type);
    const auto result_name =
        operand_view.has_value() ? gp_register_name(target_index, *operand_view)
                                 : std::nullopt;
    const auto rhs_name =
        operand_view.has_value() ? gp_register_name(scratch_index, *operand_view)
                                 : std::nullopt;
    if (!operand_view.has_value() || !result_name.has_value() ||
        !rhs_name.has_value()) {
      return false;
    }
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    if (!emit_value_publication_to_register(
            context,
            lhs,
            binary_index,
            target_index,
            scratch_index,
            lines,
            reload_current_memory_loads)) {
      return false;
    }
    const std::uint8_t nested_scratch_index = scratch_index == 9 ? 10 : 9;
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            binary_index,
                                            scratch_index,
                                            nested_scratch_index,
                                            lines,
                                            reload_current_memory_loads)) {
      return false;
    }
    std::string_view opcode = "and";
    if (binary->opcode == bir::BinaryOpcode::Or) {
      opcode = "orr";
    } else if (binary->opcode == bir::BinaryOpcode::Xor) {
      opcode = "eor";
    }
    lines.push_back(std::string{opcode} + " " + *result_name + ", " +
                    *result_name + ", " + *rhs_name);
    return true;
  }

  if (binary->opcode == bir::BinaryOpcode::Mul) {
    const auto operand_view = scalar_view_for_type(binary->operand_type);
    const auto result_name =
        operand_view.has_value() ? gp_register_name(target_index, *operand_view)
                                 : std::nullopt;
    const auto rhs_name =
        operand_view.has_value() ? gp_register_name(scratch_index, *operand_view)
                                 : std::nullopt;
    if (!operand_view.has_value() || !result_name.has_value() ||
        !rhs_name.has_value()) {
      return false;
    }
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    if (const auto shift = value_power_of_two_shift(context, rhs, binary_index)) {
      if (!emit_value_publication_to_register(context,
                                              lhs,
                                              binary_index,
                                              target_index,
                                              scratch_index,
                                              lines,
                                              reload_current_memory_loads)) {
        return false;
      }
      if (*shift != 0U) {
        lines.push_back("lsl " + *result_name + ", " + *result_name +
                        ", #" + std::to_string(*shift));
      }
      return true;
    }
    const std::uint8_t nested_scratch_index = scratch_index == 9 ? 10 : 9;
    const bool rhs_reads_target = value_publication_may_read_register_index(
        context, rhs, binary_index, target_index);
    const bool lhs_reads_scratch = value_publication_may_read_register_index(
        context, lhs, binary_index, scratch_index);
    const bool rhs_writes_target =
        nested_scratch_index == target_index &&
        value_publication_may_write_scratch_register(context, rhs, binary_index);
    const bool lhs_writes_scratch =
        value_publication_may_write_scratch_register(context, lhs, binary_index);
    if ((rhs_reads_target || rhs_writes_target) &&
        !lhs_reads_scratch && !lhs_writes_scratch) {
      if (!emit_value_publication_to_register(context,
                                              rhs,
                                              binary_index,
                                              scratch_index,
                                              nested_scratch_index,
                                              lines,
                                              reload_current_memory_loads) ||
          !emit_value_publication_to_register(context,
                                              lhs,
                                              binary_index,
                                              target_index,
                                              scratch_index,
                                              lines,
                                              reload_current_memory_loads)) {
        return false;
      }
    } else if (!emit_value_publication_to_register(context,
                                                   lhs,
                                                   binary_index,
                                                   target_index,
                                                   scratch_index,
                                                   lines,
                                                   reload_current_memory_loads) ||
               !emit_value_publication_to_register(context,
                                                   rhs,
                                                   binary_index,
                                                   scratch_index,
                                                   nested_scratch_index,
                                                   lines,
                                                   reload_current_memory_loads)) {
      return false;
    }
    lines.push_back("mul " + *result_name + ", " + *result_name + ", " + *rhs_name);
    return true;
  }

  if (binary->opcode != bir::BinaryOpcode::Add &&
      binary->opcode != bir::BinaryOpcode::Sub) {
    return false;
  }
  if (emit_local_slot_address_publication_to_register(
          context, *binary, target_index, producer_context->instruction_index, lines)) {
    return true;
  }
  auto lhs = binary->lhs;
  lhs.type = binary->operand_type;
  auto rhs = binary->rhs;
  rhs.type = binary->operand_type;
  const std::uint8_t nested_scratch_index = scratch_index == 9 ? 10 : 9;
  const bool rhs_reads_target = value_publication_may_read_register_index(
      context, rhs, binary_index, target_index);
  const bool lhs_reads_scratch = value_publication_may_read_register_index(
      context, lhs, binary_index, scratch_index);
  const bool rhs_writes_target =
      nested_scratch_index == target_index &&
      value_publication_may_write_scratch_register(context, rhs, binary_index);
  const bool lhs_writes_scratch =
      value_publication_may_write_scratch_register(context, lhs, binary_index);
  if ((rhs_reads_target || rhs_writes_target) &&
      !lhs_reads_scratch && !lhs_writes_scratch) {
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            binary_index,
                                            scratch_index,
                                            nested_scratch_index,
                                            lines,
                                            reload_current_memory_loads) ||
        !emit_value_publication_to_register(context,
                                            lhs,
                                            binary_index,
                                            target_index,
                                            scratch_index,
                                            lines,
                                            reload_current_memory_loads)) {
      return false;
    }
  } else {
    if (!emit_value_publication_to_register(
            context,
            lhs,
            binary_index,
            target_index,
            scratch_index,
            lines,
            reload_current_memory_loads) ||
        !emit_value_publication_to_register(context,
                                            rhs,
                                            binary_index,
                                            scratch_index,
                                            nested_scratch_index,
                                            lines,
                                            reload_current_memory_loads)) {
      return false;
    }
  }
  const auto result = gp_register_name(target_index, abi::RegisterView::X);
  const auto rhs_name = gp_register_name(scratch_index, abi::RegisterView::X);
  if (!result.has_value() || !rhs_name.has_value()) {
    return false;
  }
  lines.push_back(std::string{binary->opcode == bir::BinaryOpcode::Add ? "add " : "sub "} +
                  *result + ", " + *result + ", " + *rhs_name);
  return true;
}
}  // namespace c4c::backend::aarch64::codegen
