#include "dispatch.hpp"


#include "../abi/abi.hpp"
#include "alu.hpp"
#include "float_ops.hpp"
#include "machine_printer.hpp"
#include "operands.hpp"

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

[[nodiscard]] bool emit_prepared_global_symbol_load_to_register(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    bir::TypeKind type,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines) {
  if (context.function.prepared == nullptr) {
    return false;
  }
  const auto* access = prepared_memory_access(context, instruction_index);
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !access->address.symbol_name.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return false;
  }
  const auto symbol_name =
      prepare::prepared_link_name(context.function.prepared->names,
                                  *access->address.symbol_name);
  const auto mnemonic = scalar_load_mnemonic(type);
  const auto target_view = scalar_view_for_type(type);
  const auto target = target_view.has_value()
                          ? gp_register_name(target_index, *target_view)
                          : std::nullopt;
  const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
  if (symbol_name.empty() || !mnemonic.has_value() || !target.has_value() ||
      !address.has_value()) {
    return false;
  }
  const auto symbol =
      relocation_operand(symbol_name,
                         static_cast<std::size_t>(access->address.byte_offset));
  lines.push_back("adrp " + *address + ", " + symbol);
  lines.push_back("add " + *address + ", " + *address + ", :lo12:" + symbol);
  lines.push_back(std::string{*mnemonic} + " " + *target + ", [" + *address + "]");
  return true;
}
[[nodiscard]] bool emit_fp_immediate_to_register(std::vector<std::string>& lines,
                                                 const bir::Value& value,
                                                 abi::RegisterReference fp_destination,
                                                 std::uint8_t gp_scratch_index) {
  if (value.kind != bir::Value::Kind::Immediate) {
    return false;
  }
  const auto fp_view = scalar_fp_register_view(fp_destination, value.type);
  if (!fp_view.has_value()) {
    return false;
  }
  if (value.type == bir::TypeKind::F32) {
    const auto gp_scratch = abi::gp_register(gp_scratch_index, abi::RegisterView::W);
    if (!gp_scratch.has_value()) {
      return false;
    }
    auto materialize =
        materialize_integer_constant_lines(*gp_scratch, value.immediate_bits, 32U);
    if (materialize.empty()) {
      return false;
    }
    lines.insert(lines.end(), materialize.begin(), materialize.end());
    lines.push_back("fmov " + std::string{abi::register_name(*fp_view)} + ", " +
                    std::string{abi::register_name(*gp_scratch)});
    return true;
  }
  if (value.type == bir::TypeKind::F64) {
    const auto gp_scratch = abi::gp_register(gp_scratch_index, abi::RegisterView::X);
    if (!gp_scratch.has_value()) {
      return false;
    }
    auto materialize =
        materialize_integer_constant_lines(*gp_scratch, value.immediate_bits, 64U);
    if (materialize.empty()) {
      return false;
    }
    lines.insert(lines.end(), materialize.begin(), materialize.end());
    lines.push_back("fmov " + std::string{abi::register_name(*fp_view)} + ", " +
                    std::string{abi::register_name(*gp_scratch)});
    return true;
  }
  return false;
}
[[nodiscard]] bool emit_fp_value_to_register(const module::BlockLoweringContext& context,
                                             const bir::Value& value,
                                             std::size_t before_instruction_index,
                                             abi::RegisterReference destination,
                                             std::uint8_t gp_scratch_index,
                                             std::vector<std::string>& lines) {
  const auto destination_view = scalar_fp_register_view(destination, value.type);
  if (!destination_view.has_value()) {
    return false;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    return emit_fp_immediate_to_register(
        lines, value, *destination_view, gp_scratch_index);
  }
  if (value.kind != bir::Value::Kind::Named) {
    return false;
  }
  const auto* producer =
      find_same_block_named_producer(context, value.name, before_instruction_index);
  if (const auto* binary =
          producer != nullptr ? std::get_if<bir::BinaryInst>(producer) : nullptr;
      binary != nullptr && is_prepared_scalar_float_alu_operation(*binary)) {
    const auto producer_index =
        producer_instruction_index(context, producer).value_or(before_instruction_index);
    std::string_view mnemonic;
    switch (binary->opcode) {
      case bir::BinaryOpcode::Add:
        mnemonic = "fadd";
        break;
      case bir::BinaryOpcode::Sub:
        mnemonic = "fsub";
        break;
      case bir::BinaryOpcode::Mul:
        mnemonic = "fmul";
        break;
      case bir::BinaryOpcode::SDiv:
      case bir::BinaryOpcode::UDiv:
        mnemonic = "fdiv";
        break;
      default:
        return false;
    }
    std::optional<abi::RegisterReference> rhs_scratch;
    for (const auto scratch : abi::reserved_mir_scratch_fp_simd_registers()) {
      if (scratch.index == destination_view->index) {
        continue;
      }
      const auto viewed = scalar_fp_register_view(scratch, binary->operand_type);
      if (!viewed.has_value()) {
        continue;
      }
      rhs_scratch = *viewed;
      break;
    }
    if (!rhs_scratch.has_value()) {
      return false;
    }
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    if (!emit_fp_value_to_register(context,
                                   lhs,
                                   producer_index,
                                   *destination_view,
                                   gp_scratch_index,
                                   lines) ||
        !emit_fp_value_to_register(context,
                                   rhs,
                                   producer_index,
                                   *rhs_scratch,
                                   gp_scratch_index,
                                   lines)) {
      return false;
    }
    lines.push_back(std::string{mnemonic} + " " +
                    std::string{abi::register_name(*destination_view)} + ", " +
                    std::string{abi::register_name(*destination_view)} + ", " +
                    std::string{abi::register_name(*rhs_scratch)});
    return true;
  }
  if (const auto* select =
          producer != nullptr ? std::get_if<bir::SelectInst>(producer) : nullptr;
      select != nullptr) {
    const auto condition = branch_condition_suffix(select->predicate);
    const auto compare_view = scalar_view_for_type(select->compare_type);
    if (!condition.has_value() || !compare_view.has_value()) {
      return false;
    }
    const auto producer_index =
        producer_instruction_index(context, producer).value_or(before_instruction_index);
    const auto gp_lhs = abi::gp_register(gp_scratch_index, *compare_view);
    const std::uint8_t gp_rhs_index = gp_scratch_index == 9 ? 10 : 9;
    const auto gp_rhs = abi::gp_register(gp_rhs_index, *compare_view);
    if (!gp_lhs.has_value() || !gp_rhs.has_value()) {
      return false;
    }
    std::optional<abi::RegisterReference> true_scratch;
    for (const auto scratch : abi::reserved_mir_scratch_fp_simd_registers()) {
      if (scratch.index == destination.index) {
        continue;
      }
      true_scratch = scalar_fp_register_view(scratch, value.type);
      if (true_scratch.has_value()) {
        break;
      }
    }
    if (!true_scratch.has_value()) {
      return false;
    }
    auto true_value = select->true_value;
    true_value.type = value.type;
    auto false_value = select->false_value;
    false_value.type = value.type;
    if (!emit_fp_value_to_register(
            context, false_value, producer_index, *destination_view, gp_scratch_index, lines) ||
        !emit_fp_value_to_register(
            context, true_value, producer_index, *true_scratch, gp_scratch_index, lines)) {
      return false;
    }
    auto lhs = select->lhs;
    lhs.type = select->compare_type;
    if (!emit_value_publication_to_register(
            context, lhs, producer_index, gp_scratch_index, gp_rhs_index, lines)) {
      return false;
    }
    std::string rhs_name;
    if (select->rhs.kind == bir::Value::Kind::Immediate &&
        is_cmp_immediate_encodable(select->rhs.immediate)) {
      rhs_name = "#" + std::to_string(select->rhs.immediate);
    } else {
      auto rhs = select->rhs;
      rhs.type = select->compare_type;
      if (!emit_value_publication_to_register(
              context, rhs, producer_index, gp_rhs_index, gp_scratch_index, lines)) {
        return false;
      }
      rhs_name = abi::register_name(*gp_rhs);
    }
    lines.push_back("cmp " + std::string{abi::register_name(*gp_lhs)} + ", " + rhs_name);
    lines.push_back("fcsel " + std::string{abi::register_name(*destination_view)} + ", " +
                    std::string{abi::register_name(*true_scratch)} + ", " +
                    std::string{abi::register_name(*destination_view)} + ", " +
                    std::string{*condition});
    return true;
  }
  if (const auto* load_local =
          producer != nullptr ? std::get_if<bir::LoadLocalInst>(producer) : nullptr;
      load_local != nullptr) {
    const auto producer_index = producer_instruction_index(context, producer);
    const auto offset = producer_index.has_value()
                            ? prepared_local_load_offset(context, *producer_index)
                            : std::nullopt;
    if (offset.has_value()) {
      lines.push_back("ldr " + std::string{abi::register_name(*destination_view)} +
                      ", " + frame_slot_address(context.function, *offset));
      return true;
    }
  }
  if (const auto* load_global =
          producer != nullptr ? std::get_if<bir::LoadGlobalInst>(producer) : nullptr;
      load_global != nullptr) {
    const auto address = abi::gp_register(gp_scratch_index, abi::RegisterView::X);
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
      lines.push_back("adrp " + std::string{abi::register_name(*address)} + ", :got:" +
                      symbol_label);
      lines.push_back("ldr " + std::string{abi::register_name(*address)} + ", [" +
                      std::string{abi::register_name(*address)} + ", :got_lo12:" +
                      symbol_label + "]");
      lines.push_back("ldr " + std::string{abi::register_name(*destination_view)} + ", " +
                      register_indirect_address(abi::register_name(*address),
                                                load_global->byte_offset));
      return true;
    }
    const auto symbol = relocation_operand(symbol_label, load_global->byte_offset);
    lines.push_back("adrp " + std::string{abi::register_name(*address)} + ", " + symbol);
    lines.push_back("add " + std::string{abi::register_name(*address)} + ", " +
                    std::string{abi::register_name(*address)} + ", :lo12:" + symbol);
    lines.push_back("ldr " + std::string{abi::register_name(*destination_view)} + ", [" +
                    std::string{abi::register_name(*address)} + "]");
    return true;
  }
  if (const auto* cast =
          producer != nullptr ? std::get_if<bir::CastInst>(producer) : nullptr;
      cast != nullptr) {
    const auto producer_index =
        producer_instruction_index(context, producer).value_or(before_instruction_index);
    switch (cast->opcode) {
      case bir::CastOpcode::SIToFP:
      case bir::CastOpcode::UIToFP: {
        const auto source_width = scalar_integer_width_bits(cast->operand.type);
        const auto gp_source =
            source_width.has_value()
                ? abi::gp_register(gp_scratch_index,
                                   *source_width == 64U ? abi::RegisterView::X
                                                        : abi::RegisterView::W)
                : std::nullopt;
        if (!source_width.has_value() || !gp_source.has_value()) {
          return false;
        }
        if (cast->operand.kind == bir::Value::Kind::Immediate) {
          auto materialize = materialize_integer_constant_lines(
              *gp_source, immediate_integer_bits(cast->operand, *source_width), *source_width);
          if (materialize.empty()) {
            return false;
          }
          lines.insert(lines.end(), materialize.begin(), materialize.end());
        } else if (!emit_value_publication_to_register(context,
                                                       cast->operand,
                                                       producer_index,
                                                       gp_scratch_index,
                                                       destination.index,
                                                       lines)) {
          return false;
        }
        lines.push_back(std::string{cast->opcode == bir::CastOpcode::SIToFP ? "scvtf "
                                                                            : "ucvtf "} +
                        std::string{abi::register_name(*destination_view)} + ", " +
                        std::string{abi::register_name(*gp_source)});
        return true;
      }
      case bir::CastOpcode::FPExt:
      case bir::CastOpcode::FPTrunc: {
        std::optional<abi::RegisterReference> fp_source_base;
        for (const auto scratch : abi::reserved_mir_scratch_fp_simd_registers()) {
          if (scratch.index != destination.index) {
            fp_source_base = abi::fp_simd_register(scratch.index, abi::RegisterView::D);
            break;
          }
        }
        if (!fp_source_base.has_value() ||
            !emit_fp_value_to_register(context,
                                       cast->operand,
                                       producer_index,
                                       *fp_source_base,
                                       gp_scratch_index,
                                       lines)) {
          return false;
        }
        const auto fp_source = scalar_fp_register_view(*fp_source_base, cast->operand.type);
        if (!fp_source.has_value()) {
          return false;
        }
        lines.push_back("fcvt " + std::string{abi::register_name(*destination_view)} +
                        ", " + std::string{abi::register_name(*fp_source)});
        return true;
      }
      case bir::CastOpcode::FPToSI:
      case bir::CastOpcode::FPToUI:
      case bir::CastOpcode::SExt:
      case bir::CastOpcode::ZExt:
      case bir::CastOpcode::Trunc:
      case bir::CastOpcode::PtrToInt:
      case bir::CastOpcode::IntToPtr:
      case bir::CastOpcode::Bitcast:
        break;
    }
  }
  const auto* home = prepared_value_home_for_value(context, value);
  if (home == nullptr) {
    return false;
  }
  if (home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value()) {
    lines.push_back("ldr " + std::string{abi::register_name(*destination_view)} +
                    ", " + frame_slot_address(context.function, *home->offset_bytes));
    return true;
  }
  if (home->kind == prepare::PreparedValueHomeKind::Register &&
      home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
    if (!parsed.has_value() || !abi::is_fp_simd_register(*parsed)) {
      return false;
    }
    const auto source = scalar_fp_register_view(*parsed, value.type);
    if (!source.has_value()) {
      return false;
    }
    const std::string source_name = abi::register_name(*source);
    const std::string destination_name = abi::register_name(*destination_view);
    if (source_name != destination_name) {
      lines.push_back("fmov " + destination_name + ", " + source_name);
    }
    return true;
  }
  return false;
}
[[nodiscard]] bool emit_prepared_va_list_field_load_to_register(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load_local,
    std::uint8_t target_index,
    std::vector<std::string>& lines) {
  const auto address = prepared_va_list_field_address(context, load_local.slot_name);
  if (!address.has_value()) {
    return false;
  }
  const auto mnemonic = scalar_load_mnemonic(load_local.result.type);
  const auto target_view = scalar_view_for_type(load_local.result.type);
  const auto target =
      target_view.has_value() ? gp_register_name(target_index, *target_view) : std::nullopt;
  if (!mnemonic.has_value() || !target.has_value()) {
    return false;
  }
  lines.push_back(std::string{*mnemonic} + " " + *target + ", " + *address);
  return true;
}
[[nodiscard]] bool emit_prepared_pointer_value_load_to_register(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load_local,
    std::size_t instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
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
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return false;
  }
  const auto mnemonic = scalar_load_mnemonic(load_local.result.type);
  const auto target_view = scalar_view_for_type(load_local.result.type);
  const auto target =
      target_view.has_value() ? gp_register_name(target_index, *target_view) : std::nullopt;
  const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
  if (!mnemonic.has_value() || !target.has_value() || !address.has_value()) {
    return false;
  }
  const auto* pointer_home =
      find_value_home(context,
*access->address.pointer_value_name);
  if (pointer_home == nullptr) {
    return false;
  }
  if (pointer_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      pointer_home->offset_bytes.has_value()) {
    if (is_byval_formal_value_name(context, *access->address.pointer_value_name)) {
      const auto offset =
          static_cast<std::int64_t>(*pointer_home->offset_bytes) +
          access->address.byte_offset;
      if (offset < 0) {
        return false;
      }
      lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                      frame_slot_address(context.function,
                                         static_cast<std::size_t>(offset)));
      return true;
    }
    lines.push_back("ldr " + *address + ", " +
                    frame_slot_address(context.function, *pointer_home->offset_bytes));
  } else if (pointer_home->kind == prepare::PreparedValueHomeKind::Register &&
             pointer_home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*pointer_home->register_name);
    if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
      return false;
    }
    const auto viewed = abi::gp_register(parsed->index, abi::RegisterView::X);
    if (!viewed.has_value()) {
      return false;
    }
    const auto source = abi::register_name(*viewed);
    if (source != *address) {
      lines.push_back("mov " + *address + ", " + std::string{source});
    }
  } else {
    return false;
  }
  lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                  register_indirect_address(*address,
                                            static_cast<std::size_t>(
                                                access->address.byte_offset)));
  return true;
}
[[nodiscard]] bool emit_prepared_value_home_to_register(
    const prepare::PreparedStackLayout* stack_layout,
    const prepare::PreparedValueHome& home,
    bir::TypeKind type,
    std::uint8_t target_index,
    std::vector<std::string>& lines,
    bool use_frame_pointer_base) {
  const auto target_view = scalar_view_for_type(type);
  if (!target_view.has_value()) {
    return false;
  }
  const auto target = gp_register_name(target_index, *target_view);
  if (!target.has_value()) {
    return false;
  }
  if (home.kind == prepare::PreparedValueHomeKind::StackSlot &&
      home.offset_bytes.has_value()) {
    const auto type_width = dispatch_publication_scalar_type_size_bytes(type);
    if (!type_width.has_value()) {
      return false;
    }
    std::optional<std::size_t> home_width = home.size_bytes;
    if (stack_layout != nullptr && home.slot_id.has_value()) {
      if (const auto* slot = find_frame_slot(*stack_layout, *home.slot_id); slot != nullptr) {
        home_width = home_width.has_value()
                         ? std::min(*home_width, slot->size_bytes)
                         : std::optional<std::size_t>{slot->size_bytes};
      }
    }
    const auto load_width =
        home_width.has_value()
            ? std::min(*home_width, *type_width)
            : *type_width;
    const auto mnemonic = scalar_load_mnemonic_for_width(load_width);
    const auto load_view = load_width == 8 ? abi::RegisterView::X
                                           : abi::RegisterView::W;
    const auto load_target = gp_register_name(target_index, load_view);
    if (!mnemonic.has_value() || !load_target.has_value()) {
      return false;
    }
    lines.push_back(std::string{*mnemonic} + " " + *load_target + ", " +
                    frame_slot_address(*home.offset_bytes,
                                       use_frame_pointer_base ? "x29" : "sp"));
    return true;
  }
  if (home.kind == prepare::PreparedValueHomeKind::Register &&
      home.register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*home.register_name);
    if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
      return false;
    }
    const auto source = abi::gp_register(parsed->index, *target_view);
    if (!source.has_value()) {
      return false;
    }
    const auto source_name = std::string{abi::register_name(*source)};
    if (source_name != *target) {
      lines.push_back("mov " + *target + ", " + source_name);
    }
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
  if (const auto constant = evaluate_same_block_integer_constant(
          context, value);
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
  const auto* producer =
      find_same_block_named_producer(context, value.name, before_instruction_index);
  if (producer != nullptr &&
      is_current_block_join_parallel_copy_source(context, *producer)) {
    const auto* home = prepared_value_home_for_value(context, value);
    if (home != nullptr) {
      return emit_prepared_value_home_to_register(context.function.prepared != nullptr
                                                     ? &context.function.prepared->stack_layout
                                                     : nullptr,
                                                 *home,
                                                 value.type,
                                                 target_index,
                                                 lines,
                                                 fixed_slots_use_frame_pointer(context.function));
    }
  }
  if (producer == nullptr) {
    const auto* home = prepared_value_home_for_value(context, value);
    if (home != nullptr) {
      return emit_prepared_value_home_to_register(context.function.prepared != nullptr
                                                     ? &context.function.prepared->stack_layout
                                                     : nullptr,
                                                 *home,
                                                 value.type,
                                                 target_index,
                                                 lines,
                                                 fixed_slots_use_frame_pointer(context.function));
    }
    return false;
  }
  if (const auto* home = prepared_value_home_for_value(context, value);
      home != nullptr && value_has_current_block_entry_publication(context, *home)) {
    return emit_prepared_value_home_to_register(context.function.prepared != nullptr
                                                   ? &context.function.prepared->stack_layout
                                                   : nullptr,
                                               *home,
                                               value.type,
                                               target_index,
                                               lines,
                                               fixed_slots_use_frame_pointer(context.function));
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
lower_scalar_cast_publication_to_prepared_register(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto* cast = std::get_if<bir::CastInst>(&inst);
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  const auto result = instruction_result_value(inst);
  const bool supported_binary =
      binary != nullptr &&
      branch_condition_suffix(binary->opcode).has_value() &&
      (binary->operand_type == bir::TypeKind::F32 ||
       binary->operand_type == bir::TypeKind::F64);
  if ((cast == nullptr && !supported_binary) || !result.has_value()) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, *result);
  if (!value_name.has_value() ||
      find_emitted_scalar_register(scalar_state, *value_name).has_value()) {
    return std::nullopt;
  }
  const auto* home = prepared_value_home_for_value(context, *result);
  if (home == nullptr || home->kind != prepare::PreparedValueHomeKind::Register ||
      !home->register_name.has_value()) {
    return std::nullopt;
  }
  const auto* storage = [&]() -> const prepare::PreparedStoragePlanValue* {
    if (context.function.storage_plan == nullptr) {
      return nullptr;
    }
    for (const auto& value : context.function.storage_plan->values) {
      if (value.value_id == home->value_id) {
        return &value;
      }
    }
    return nullptr;
  }();
  if (storage == nullptr ||
      storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      storage->register_name != home->register_name) {
    return std::nullopt;
  }
  const auto expected_view = scalar_view_for_type(result->type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
  if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto target_register = abi::gp_register(parsed->index, *expected_view);
  if (!target_register.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::optional<std::uint8_t> scratch_index;
  for (const auto& scratch : scratches) {
    if (scratch.index != parsed->index) {
      scratch_index = scratch.index;
      break;
    }
  }
  if (!scratch_index.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  if (!emit_value_publication_to_register(context,
                                          *result,
                                          instruction_index + 1,
                                          parsed->index,
                                          *scratch_index,
                                          lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  RegisterOperand emitted{
      .reg = *target_register,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = home->value_id,
      .value_name = home->value_name,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = expected_view,
  };
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}
[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_fp_binary_publication_to_prepared_register(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr || !is_prepared_scalar_float_alu_operation(*binary)) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, binary->result);
  if (!value_name.has_value() ||
      find_emitted_scalar_register(scalar_state, *value_name).has_value()) {
    return std::nullopt;
  }
  const auto* home = prepared_value_home_for_value(context, binary->result);
  if (home == nullptr || home->kind != prepare::PreparedValueHomeKind::Register ||
      !home->register_name.has_value()) {
    return std::nullopt;
  }
  const auto* storage = [&]() -> const prepare::PreparedStoragePlanValue* {
    if (context.function.storage_plan == nullptr) {
      return nullptr;
    }
    for (const auto& value : context.function.storage_plan->values) {
      if (value.value_id == home->value_id) {
        return &value;
      }
    }
    return nullptr;
  }();
  if (storage == nullptr ||
      storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      storage->bank != prepare::PreparedRegisterBank::Fpr ||
      (!storage->register_placement.has_value() &&
       (!storage->register_name.has_value() ||
        *storage->register_name != *home->register_name))) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
  if (!parsed.has_value() || !abi::is_fp_simd_register(*parsed)) {
    return std::nullopt;
  }
  const auto result_register = scalar_fp_register_view(*parsed, binary->result.type);
  if (!result_register.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  if (!emit_fp_value_to_register(context,
                                 binary->result,
                                 instruction_index + 1,
                                 *result_register,
                                 scratches.front().index,
                                 lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  RegisterOperand emitted{
      .reg = *result_register,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = home->value_id,
      .value_name = home->value_name,
      .prepared_class = prepare::PreparedRegisterClass::Float,
      .prepared_bank = prepare::PreparedRegisterBank::Fpr,
      .expected_view = result_register->view,
  };
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}
[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_cast_publication_to_prepared_stack(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state) {
  const auto* cast = std::get_if<bir::CastInst>(&inst);
  const auto result = instruction_result_value(inst);
  if (cast == nullptr || !result.has_value()) {
    return std::nullopt;
  }
  const auto* home = prepared_value_home_for_value(context, *result);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home->offset_bytes.has_value()) {
    return std::nullopt;
  }
  const auto mnemonic = scalar_store_mnemonic(result->type);
  const auto value_view = scalar_view_for_type(result->type);
  if (!mnemonic.has_value() || !value_view.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.size() < 2U) {
    return std::nullopt;
  }
  const auto target = gp_register_name(scratches.front().index, *value_view);
  if (!target.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  const auto source_bits = integer_bit_width(cast->operand.type);
  const auto result_bits = integer_bit_width(cast->result.type);
  if (cast->opcode == bir::CastOpcode::ZExt && source_bits.has_value() &&
      result_bits.has_value() && *source_bits == *result_bits &&
      cast->operand.kind == bir::Value::Kind::Named) {
    const auto source_name = prepared_named_value_id(context, cast->operand);
    const auto source_view = scalar_view_for_type(cast->operand.type);
    const auto emitted_source =
        source_name.has_value()
            ? find_emitted_scalar_register(scalar_state, *source_name)
            : std::nullopt;
    if (source_view.has_value() && emitted_source.has_value() &&
        emitted_source->reg.bank == abi::RegisterBank::GeneralPurpose) {
      const auto source_register =
          abi::gp_register(emitted_source->reg.index, *source_view);
      if (source_register.has_value()) {
        lines.push_back(std::string{*mnemonic} + " " +
                        std::string{abi::register_name(*source_register)} + ", " +
                        frame_slot_address(context.function, *home->offset_bytes));
        return make_select_chain_materialization_instruction(
            context, instruction_index, std::move(lines));
      }
    }
  }
  if (!emit_value_publication_to_register(context,
                                          *result,
                                          instruction_index + 1,
                                          scratches.front().index,
                                          scratches[1].index,
                                          lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                  frame_slot_address(context.function, *home->offset_bytes));
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}

}  // namespace c4c::backend::aarch64::codegen
