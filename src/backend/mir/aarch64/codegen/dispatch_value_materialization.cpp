#include "dispatch_value_materialization.hpp"

#include "dispatch.hpp"
#include "dispatch_edge_copies.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "comparison_branch_fusion.hpp"
#include "constant_materialization.hpp"
#include "dispatch_lookup.hpp"
#include "dispatch_producers.hpp"
#include "dispatch_publication.hpp"
#include "dispatch_publication_common.hpp"
#include "fp_value_materialization.hpp"
#include "float_ops.hpp"
#include "globals.hpp"
#include "memory_store_sources.hpp"
#include "operands.hpp"
#include "prepared_value_home_materialization.hpp"
#include "variadic.hpp"

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
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

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
          mir::evaluate_same_block_integer_constant(context.bir_block, value);
      constant.has_value()) {
    lines.push_back("mov " + *target + ", #" + std::to_string(constant->value));
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
  const auto* producer =
      find_same_block_named_producer(context, value.name, before_instruction_index);
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
    const auto index = producer_instruction_index(context, producer);
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
    if (index.has_value() &&
        emit_prepared_global_symbol_load_to_register(context,
                                                     *index,
                                                     load_local->result.type,
                                                     target_index,
                                                     scratch_index,
                                                     lines)) {
      return true;
    }
    if (index.has_value()) {
      const auto narrow_store =
          find_latest_narrow_store_for_wide_local_load(context, *load_local, *index);
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
    }
    const auto offset =
        index.has_value() ? prepared_local_load_offset(context, *index) : std::nullopt;
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
    return index.has_value() &&
           emit_prepared_pointer_value_load_to_register(
               context, *load_local, *index, target_index, scratch_index, lines);
  }

  if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(producer);
      load_global != nullptr) {
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
    const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
    if (!address.has_value() || load_global->global_name.empty()) {
      return false;
    }
    const bir::Global* target_global = find_load_global_target(context, *load_global);
    const auto symbol_label = load_global_symbol_label(context, *load_global, target_global);
    if (symbol_label.empty()) {
      return false;
    }
    if (target_global != nullptr &&
        target_global->address_materialization_policy ==
            bir::GlobalAddressMaterializationPolicy::GotRequired) {
      lines.push_back("adrp " + *address + ", :got:" + symbol_label);
      lines.push_back("ldr " + *address + ", [" + *address + ", :got_lo12:" +
                      symbol_label + "]");
      lines.push_back("ldr " + *target + ", " +
                      register_indirect_address(*address, load_global->byte_offset));
      return true;
    }
    const auto symbol = relocation_operand(symbol_label, load_global->byte_offset);
    lines.push_back("adrp " + *address + ", " + symbol);
    lines.push_back("add " + *address + ", " + *address + ", :lo12:" + symbol);
    lines.push_back("ldr " + *target + ", [" + *address + "]");
    return true;
  }

  if (const auto* cast = std::get_if<bir::CastInst>(producer); cast != nullptr) {
    const auto cast_index =
        producer_instruction_index(context, producer).value_or(before_instruction_index);
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
    std::size_t label_index = 0;
    std::vector<std::string_view> active_values;
    return emit_select_chain_value_to_register(context,
                                               value,
                                               before_instruction_index,
                                               target_index,
                                               scratch_index,
                                               before_instruction_index,
                                               prepared_named_value_id(context, value)
                                                   .value_or(c4c::kInvalidValueName),
                                               lines,
                                               label_index,
                                               active_values,
                                               reload_current_memory_loads);
  }

  const auto* binary = std::get_if<bir::BinaryInst>(producer);
  if (binary == nullptr) {
    return false;
  }

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
                                     before_instruction_index,
                                     *lhs_fp,
                                     scratch_index,
                                     lines) ||
          !emit_fp_value_to_register(context,
                                     rhs,
                                     before_instruction_index,
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
            before_instruction_index,
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
                                              before_instruction_index,
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
            before_instruction_index,
            target_index,
            scratch_index,
            lines,
            reload_current_memory_loads)) {
      return false;
    }
    const std::uint8_t nested_scratch_index = scratch_index == 9 ? 10 : 9;
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            before_instruction_index,
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
    if (rhs.kind == bir::Value::Kind::Immediate && rhs.immediate >= 0) {
      if (const auto shift =
              power_of_two_shift(static_cast<std::uint64_t>(rhs.immediate))) {
        if (!emit_value_publication_to_register(context,
                                                lhs,
                                                before_instruction_index,
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
    }
    const std::uint8_t nested_scratch_index = scratch_index == 9 ? 10 : 9;
    const bool rhs_reads_target = value_publication_may_read_register_index(
        context, rhs, before_instruction_index, target_index);
    const bool lhs_reads_scratch = value_publication_may_read_register_index(
        context, lhs, before_instruction_index, scratch_index);
    if (rhs_reads_target && !lhs_reads_scratch) {
      if (!emit_value_publication_to_register(context,
                                              rhs,
                                              before_instruction_index,
                                              scratch_index,
                                              nested_scratch_index,
                                              lines,
                                              reload_current_memory_loads) ||
          !emit_value_publication_to_register(context,
                                              lhs,
                                              before_instruction_index,
                                              target_index,
                                              scratch_index,
                                              lines,
                                              reload_current_memory_loads)) {
        return false;
      }
    } else if (!emit_value_publication_to_register(context,
                                                   lhs,
                                                   before_instruction_index,
                                                   target_index,
                                                   scratch_index,
                                                   lines,
                                                   reload_current_memory_loads) ||
               !emit_value_publication_to_register(context,
                                                   rhs,
                                                   before_instruction_index,
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
          context, *binary, target_index, lines)) {
    return true;
  }
  auto lhs = binary->lhs;
  lhs.type = binary->operand_type;
  auto rhs = binary->rhs;
  rhs.type = binary->operand_type;
  const std::uint8_t nested_scratch_index = scratch_index == 9 ? 10 : 9;
  const bool rhs_reads_target = value_publication_may_read_register_index(
      context, rhs, before_instruction_index, target_index);
  const bool lhs_reads_scratch = value_publication_may_read_register_index(
      context, lhs, before_instruction_index, scratch_index);
  if (rhs_reads_target && !lhs_reads_scratch) {
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            before_instruction_index,
                                            scratch_index,
                                            nested_scratch_index,
                                            lines,
                                            reload_current_memory_loads) ||
        !emit_value_publication_to_register(context,
                                            lhs,
                                            before_instruction_index,
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
            before_instruction_index,
            target_index,
            scratch_index,
            lines,
            reload_current_memory_loads) ||
        !emit_value_publication_to_register(context,
                                            rhs,
                                            before_instruction_index,
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
  if (!emit_local_slot_address_publication_to_register(
          context, *binary, result_register->reg.index, lines)) {
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
[[nodiscard]] std::optional<module::MachineInstruction>
lower_stack_homed_pointer_value_load_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
  if (load == nullptr || context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto* access = prepared_memory_access(context, instruction_index);
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value()) {
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

  const auto result_name = prepared_named_value_id(context, load->result);
  if (!result_name.has_value()) {
    return std::nullopt;
  }
  const auto* result_home =
      find_value_home(context,
*result_name);
  if (result_home == nullptr) {
    return std::nullopt;
  }
  const auto target_view = scalar_view_for_type(load->result.type);
  if (!target_view.has_value()) {
    return std::nullopt;
  }

  std::optional<std::uint8_t> target_index;
  if (result_home->kind == prepare::PreparedValueHomeKind::Register &&
      result_home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*result_home->register_name);
    if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
      return std::nullopt;
    }
    target_index = parsed->index;
  } else if (result_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
             result_home->offset_bytes.has_value()) {
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (scratches.empty()) {
      return std::nullopt;
    }
    target_index = scratches.front().index;
  } else {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::optional<std::uint8_t> scratch_index;
  for (const auto& scratch : scratches) {
    if (scratch.index != *target_index) {
      scratch_index = scratch.index;
      break;
    }
  }
  if (!scratch_index.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  if (!emit_prepared_pointer_value_load_to_register(
          context, *load, instruction_index, *target_index, *scratch_index, lines) ||
      lines.empty()) {
    return std::nullopt;
  }

  if (result_home->kind == prepare::PreparedValueHomeKind::StackSlot) {
    const auto mnemonic = scalar_store_mnemonic(load->result.type);
    const auto target = gp_register_name(*target_index, *target_view);
    if (!mnemonic.has_value() || !target.has_value() ||
        !result_home->offset_bytes.has_value()) {
      return std::nullopt;
    }
    lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                    frame_slot_address(context.function, *result_home->offset_bytes));
  } else if (result_home->kind == prepare::PreparedValueHomeKind::Register) {
    const auto reg = abi::gp_register(*target_index, *target_view);
    if (!reg.has_value()) {
      return std::nullopt;
    }
    RegisterOperand emitted{
        .reg = *reg,
        .role = RegisterOperandRole::StoragePlan,
        .value_id = result_home->value_id,
        .value_name = result_home->value_name,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = target_view,
    };
    record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  }

  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}
[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_mul_with_distinct_rhs_scratch(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr ||
      binary->opcode != bir::BinaryOpcode::Mul ||
      binary->result.kind != bir::Value::Kind::Named ||
      binary->result.name.empty()) {
    return std::nullopt;
  }
  if (binary->lhs.kind != bir::Value::Kind::Immediate &&
      binary->rhs.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  const auto result_view = scalar_view_for_type(binary->result.type);
  const auto operand_view = scalar_view_for_type(binary->operand_type);
  if (!result_view.has_value() || !operand_view.has_value() ||
      result_view != operand_view) {
    return std::nullopt;
  }
  auto result_register = make_named_prepared_result_register(context, binary->result);
  std::optional<std::size_t> result_stack_offset_bytes;
  if (!result_register.has_value()) {
    const auto value_name = prepared_named_value_id(context, binary->result);
    const auto* home =
        value_name.has_value() && context.function.value_locations != nullptr
            ? find_value_home(context, *value_name)
            : nullptr;
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (home == nullptr ||
        home->kind != prepare::PreparedValueHomeKind::StackSlot ||
        !home->offset_bytes.has_value() ||
        scratches.empty()) {
      return std::nullopt;
    }
    const auto scratch = abi::gp_register(scratches.front().index, *result_view);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    result_register = RegisterOperand{
        .reg = *scratch,
        .role = RegisterOperandRole::SpillAuthority,
        .value_id = home->value_id,
        .value_name = home->value_name,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = result_view,
    };
    result_stack_offset_bytes = *home->offset_bytes;
  }
  if (!abi::is_gp_register(result_register->reg)) {
    return std::nullopt;
  }
  const auto result_name =
      gp_register_name(result_register->reg.index, *result_view);
  if (!result_name.has_value()) {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::optional<std::uint8_t> rhs_scratch_index;
  std::optional<std::uint8_t> nested_scratch_index;
  for (const auto& scratch : scratches) {
    if (scratch.index != result_register->reg.index) {
      rhs_scratch_index = scratch.index;
      break;
    }
  }
  if (!rhs_scratch_index.has_value()) {
    return std::nullopt;
  }
  for (const auto& scratch : scratches) {
    if (scratch.index != result_register->reg.index &&
        scratch.index != *rhs_scratch_index) {
      nested_scratch_index = scratch.index;
      break;
    }
  }
  if (!nested_scratch_index.has_value()) {
    nested_scratch_index = result_register->reg.index;
  }
  const auto rhs_name = gp_register_name(*rhs_scratch_index, *operand_view);
  if (!rhs_name.has_value()) {
    return std::nullopt;
  }

  auto lhs = binary->lhs;
  lhs.type = binary->operand_type;
  auto rhs = binary->rhs;
  rhs.type = binary->operand_type;
  std::vector<std::string> lines;
  const auto emit_shifted_power_of_two_operand =
      [&](const bir::Value& value, std::uint64_t scale) -> bool {
    const auto shift = power_of_two_shift(scale);
    if (!shift.has_value() ||
        !emit_value_publication_to_register(context,
                                            value,
                                            instruction_index,
                                            result_register->reg.index,
                                            *rhs_scratch_index,
                                            lines)) {
      return false;
    }
    if (*shift != 0U) {
      lines.push_back("lsl " + *result_name + ", " + *result_name +
                      ", #" + std::to_string(*shift));
    }
    return true;
  };
  bool emitted_power_of_two_scale = false;
  if (rhs.kind == bir::Value::Kind::Immediate && rhs.immediate >= 0) {
    emitted_power_of_two_scale =
        emit_shifted_power_of_two_operand(lhs,
                                          static_cast<std::uint64_t>(rhs.immediate));
  } else if (lhs.kind == bir::Value::Kind::Immediate && lhs.immediate >= 0) {
    emitted_power_of_two_scale =
        emit_shifted_power_of_two_operand(rhs,
                                          static_cast<std::uint64_t>(lhs.immediate));
  }
  if (emitted_power_of_two_scale) {
    if (result_stack_offset_bytes.has_value()) {
      lines.push_back("str " + *result_name + ", " +
                      frame_slot_address(context.function, *result_stack_offset_bytes));
    }
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
        .selection =
            MachineNodeStatusRecord{
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
        .payload =
            AssemblerInstructionRecord{
                .has_inline_asm_payload = true,
                .side_effects = false,
                .inline_asm_template = std::move(asm_text),
            },
    };
    record_emitted_scalar_register(scalar_state,
                                   result_register->value_name,
                                   *result_register);
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
  const bool rhs_reads_result = value_publication_may_read_register_index(
      context, rhs, instruction_index, result_register->reg.index);
  const bool lhs_reads_rhs_scratch = value_publication_may_read_register_index(
      context, lhs, instruction_index, *rhs_scratch_index);

  if (rhs_reads_result && !lhs_reads_rhs_scratch) {
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            instruction_index,
                                            *rhs_scratch_index,
                                            *nested_scratch_index,
                                            lines) ||
        !emit_value_publication_to_register(context,
                                            lhs,
                                            instruction_index,
                                            result_register->reg.index,
                                            *rhs_scratch_index,
                                            lines)) {
      return std::nullopt;
    }
  } else {
    if (!emit_value_publication_to_register(context,
                                            lhs,
                                            instruction_index,
                                            result_register->reg.index,
                                            *rhs_scratch_index,
                                            lines) ||
        !emit_value_publication_to_register(context,
                                            rhs,
                                            instruction_index,
                                            *rhs_scratch_index,
                                            *nested_scratch_index,
                                            lines)) {
      return std::nullopt;
    }
  }
  lines.push_back("mul " + *result_name + ", " + *result_name + ", " + *rhs_name);
  if (result_stack_offset_bytes.has_value()) {
    lines.push_back("str " + *result_name + ", " +
                    frame_slot_address(context.function, *result_stack_offset_bytes));
  }

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
      .selection =
          MachineNodeStatusRecord{
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
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = false,
              .inline_asm_template = std::move(asm_text),
          },
  };
  record_emitted_scalar_register(scalar_state,
                                 result_register->value_name,
                                 *result_register);
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
}  // namespace c4c::backend::aarch64::codegen
