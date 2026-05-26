#include "fp_value_materialization.hpp"

#include "../abi/abi.hpp"
#include "comparison.hpp"
#include "comparison.hpp"
#include "constant_materialization.hpp"
#include "dispatch_edge_copies.hpp"
#include "dispatch_lookup.hpp"
#include "dispatch_producers.hpp"
#include "dispatch_publication.hpp"
#include "dispatch_value_materialization.hpp"
#include "float_ops.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "operands.hpp"

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

}  // namespace c4c::backend::aarch64::codegen
