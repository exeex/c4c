#include "prealloc.hpp"
#include "regalloc/assignment.hpp"
#include "regalloc/call_moves.hpp"
#include "regalloc/call_return_abi.hpp"
#include "regalloc/classification.hpp"
#include "regalloc/intervals.hpp"
#include "regalloc/move_records.hpp"
#include "regalloc/phi_moves.hpp"
#include "regalloc/stack_slots.hpp"
#include "regalloc/storage.hpp"
#include "regalloc/value_homes.hpp"
#include "regalloc/values.hpp"
#include "target_register_profile.hpp"
#include "stack_layout/stack_layout.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace c4c::backend::prepare {

namespace {

using regalloc_detail::append_f128_constant_values_for_function;
using regalloc_detail::append_call_arg_move_resolution;
using regalloc_detail::append_call_result_move_resolution;
using regalloc_detail::append_move_resolution_record;
using regalloc_detail::append_phi_move_resolution;
using regalloc_detail::append_return_move_resolution;
using regalloc_detail::ActiveRegisterAssignment;
using regalloc_detail::choose_eviction_candidate;
using regalloc_detail::choose_register_span;
using regalloc_detail::build_prepared_value_homes;
using regalloc_detail::call_arg_destination_register_names;
using regalloc_detail::call_arg_destination_stack_offset_bytes;
using regalloc_detail::call_arg_storage_kind;
using regalloc_detail::call_result_destination_register_names;
using regalloc_detail::call_result_storage_kind;
using regalloc_detail::f128_call_arg_destination_placement;
using regalloc_detail::find_regalloc_value;
using regalloc_detail::find_f128_constant_regalloc_value;
using regalloc_detail::function_return_storage_kind;
using regalloc_detail::infer_scalar_function_return_abi;
using regalloc_detail::interval_start_sort_key;
using regalloc_detail::intervals_overlap;
using regalloc_detail::is_f128_immediate_constant;
using regalloc_detail::locate_program_point;
using regalloc_detail::materialize_register_names;
using regalloc_detail::materialize_register_placements;
using regalloc_detail::allocate_stack_slot;
using regalloc_detail::normalized_value_size;
using regalloc_detail::published_register_group_width;
using regalloc_detail::register_bank_from_class;
using regalloc_detail::resolve_call_arg_abi;
using regalloc_detail::resolve_register_class;
using regalloc_detail::resolve_register_group_width;
using regalloc_detail::storage_transfer_reason;
using regalloc_detail::value_priority;
using regalloc_detail::weighted_use_score;

[[nodiscard]] bool is_i128_div_rem_helper_opcode(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
      return true;
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return false;
  }
  return false;
}

[[nodiscard]] PreparedI128RuntimeHelperKind i128_div_rem_helper_kind(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::SDiv:
      return PreparedI128RuntimeHelperKind::SignedDiv;
    case bir::BinaryOpcode::UDiv:
      return PreparedI128RuntimeHelperKind::UnsignedDiv;
    case bir::BinaryOpcode::SRem:
      return PreparedI128RuntimeHelperKind::SignedRem;
    case bir::BinaryOpcode::URem:
      return PreparedI128RuntimeHelperKind::UnsignedRem;
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      break;
  }
  return PreparedI128RuntimeHelperKind::SignedDiv;
}

[[nodiscard]] std::string_view i128_div_rem_helper_callee(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::SDiv:
      return "__divti3";
    case bir::BinaryOpcode::UDiv:
      return "__udivti3";
    case bir::BinaryOpcode::SRem:
      return "__modti3";
    case bir::BinaryOpcode::URem:
      return "__umodti3";
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      break;
  }
  return "";
}

[[nodiscard]] bool is_f128_soft_float_helper_opcode(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
      return true;
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return false;
  }
  return false;
}

[[nodiscard]] PreparedF128RuntimeHelperKind f128_soft_float_helper_kind(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
      return PreparedF128RuntimeHelperKind::Add;
    case bir::BinaryOpcode::Sub:
      return PreparedF128RuntimeHelperKind::Sub;
    case bir::BinaryOpcode::Mul:
      return PreparedF128RuntimeHelperKind::Mul;
    case bir::BinaryOpcode::SDiv:
      return PreparedF128RuntimeHelperKind::Div;
    case bir::BinaryOpcode::Eq:
      return PreparedF128RuntimeHelperKind::Eq;
    case bir::BinaryOpcode::Ne:
      return PreparedF128RuntimeHelperKind::Ne;
    case bir::BinaryOpcode::Slt:
      return PreparedF128RuntimeHelperKind::Lt;
    case bir::BinaryOpcode::Sle:
      return PreparedF128RuntimeHelperKind::Le;
    case bir::BinaryOpcode::Sgt:
      return PreparedF128RuntimeHelperKind::Gt;
    case bir::BinaryOpcode::Sge:
      return PreparedF128RuntimeHelperKind::Ge;
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      break;
  }
  return PreparedF128RuntimeHelperKind::Add;
}

[[nodiscard]] std::string_view f128_soft_float_helper_callee(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
      return "__addtf3";
    case bir::BinaryOpcode::Sub:
      return "__subtf3";
    case bir::BinaryOpcode::Mul:
      return "__multf3";
    case bir::BinaryOpcode::SDiv:
      return "__divtf3";
    case bir::BinaryOpcode::Eq:
      return "__eqtf2";
    case bir::BinaryOpcode::Ne:
      return "__netf2";
    case bir::BinaryOpcode::Slt:
      return "__lttf2";
    case bir::BinaryOpcode::Sle:
      return "__letf2";
    case bir::BinaryOpcode::Sgt:
      return "__gttf2";
    case bir::BinaryOpcode::Sge:
      return "__getf2";
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      break;
  }
  return "";
}

[[nodiscard]] bool is_f128_float_width_conversion_cast(const bir::CastInst& cast) {
  switch (cast.opcode) {
    case bir::CastOpcode::FPExt:
      return (cast.operand.type == bir::TypeKind::F32 ||
              cast.operand.type == bir::TypeKind::F64) &&
             cast.result.type == bir::TypeKind::F128;
    case bir::CastOpcode::FPTrunc:
      return cast.operand.type == bir::TypeKind::F128 &&
             (cast.result.type == bir::TypeKind::F32 ||
              cast.result.type == bir::TypeKind::F64);
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI:
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP:
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

[[nodiscard]] PreparedF128RuntimeHelperKind f128_cast_helper_kind(
    const bir::CastInst& cast) {
  if (cast.opcode == bir::CastOpcode::FPExt && cast.operand.type == bir::TypeKind::F32 &&
      cast.result.type == bir::TypeKind::F128) {
    return PreparedF128RuntimeHelperKind::F32ToF128;
  }
  if (cast.opcode == bir::CastOpcode::FPExt && cast.operand.type == bir::TypeKind::F64 &&
      cast.result.type == bir::TypeKind::F128) {
    return PreparedF128RuntimeHelperKind::F64ToF128;
  }
  if (cast.opcode == bir::CastOpcode::FPTrunc && cast.operand.type == bir::TypeKind::F128 &&
      cast.result.type == bir::TypeKind::F32) {
    return PreparedF128RuntimeHelperKind::F128ToF32;
  }
  if (cast.opcode == bir::CastOpcode::FPTrunc && cast.operand.type == bir::TypeKind::F128 &&
      cast.result.type == bir::TypeKind::F64) {
    return PreparedF128RuntimeHelperKind::F128ToF64;
  }
  return PreparedF128RuntimeHelperKind::F32ToF128;
}

[[nodiscard]] std::string_view f128_cast_helper_callee(const bir::CastInst& cast) {
  if (cast.opcode == bir::CastOpcode::FPExt && cast.operand.type == bir::TypeKind::F32 &&
      cast.result.type == bir::TypeKind::F128) {
    return "__extendsftf2";
  }
  if (cast.opcode == bir::CastOpcode::FPExt && cast.operand.type == bir::TypeKind::F64 &&
      cast.result.type == bir::TypeKind::F128) {
    return "__extenddftf2";
  }
  if (cast.opcode == bir::CastOpcode::FPTrunc && cast.operand.type == bir::TypeKind::F128 &&
      cast.result.type == bir::TypeKind::F32) {
    return "__trunctfsf2";
  }
  if (cast.opcode == bir::CastOpcode::FPTrunc && cast.operand.type == bir::TypeKind::F128 &&
      cast.result.type == bir::TypeKind::F64) {
    return "__trunctfdf2";
  }
  return "";
}

[[nodiscard]] PreparedF128CmpResultZeroTest f128_cmp_result_zero_test(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Eq:
      return PreparedF128CmpResultZeroTest::EqualZero;
    case bir::BinaryOpcode::Ne:
      return PreparedF128CmpResultZeroTest::NotEqualZero;
    case bir::BinaryOpcode::Slt:
      return PreparedF128CmpResultZeroTest::LessThanZero;
    case bir::BinaryOpcode::Sle:
      return PreparedF128CmpResultZeroTest::LessOrEqualZero;
    case bir::BinaryOpcode::Sgt:
      return PreparedF128CmpResultZeroTest::GreaterThanZero;
    case bir::BinaryOpcode::Sge:
      return PreparedF128CmpResultZeroTest::GreaterOrEqualZero;
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      break;
  }
  return PreparedF128CmpResultZeroTest::Missing;
}

[[nodiscard]] std::size_t i128_helper_type_width_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return 1;
    case bir::TypeKind::I16:
      return 2;
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
      return 4;
    case bir::TypeKind::I64:
    case bir::TypeKind::F64:
    case bir::TypeKind::Ptr:
      return 8;
    case bir::TypeKind::I128:
    case bir::TypeKind::F128:
      return 16;
    case bir::TypeKind::Void:
      return 0;
  }
  return 0;
}

[[nodiscard]] bool is_i128_float_integer_conversion_cast(const bir::CastInst& cast) {
  switch (cast.opcode) {
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI:
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP:
      return cast.result.type == bir::TypeKind::I128 ||
             cast.operand.type == bir::TypeKind::I128;
    case bir::CastOpcode::SExt:
    case bir::CastOpcode::ZExt:
    case bir::CastOpcode::Trunc:
    case bir::CastOpcode::FPTrunc:
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      return false;
  }
  return false;
}

[[nodiscard]] bool is_supported_i128_float_integer_conversion_cast(
    const bir::CastInst& cast) {
  if (!is_i128_float_integer_conversion_cast(cast)) {
    return false;
  }
  const bool source_is_supported_float =
      cast.operand.type == bir::TypeKind::F32 || cast.operand.type == bir::TypeKind::F64;
  const bool result_is_supported_float =
      cast.result.type == bir::TypeKind::F32 || cast.result.type == bir::TypeKind::F64;
  switch (cast.opcode) {
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI:
      return source_is_supported_float && cast.result.type == bir::TypeKind::I128;
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP:
      return cast.operand.type == bir::TypeKind::I128 && result_is_supported_float;
    case bir::CastOpcode::SExt:
    case bir::CastOpcode::ZExt:
    case bir::CastOpcode::Trunc:
    case bir::CastOpcode::FPTrunc:
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      return false;
  }
  return false;
}

[[nodiscard]] PreparedI128RuntimeHelperKind i128_float_integer_conversion_helper_kind(
    bir::CastOpcode opcode) {
  switch (opcode) {
    case bir::CastOpcode::FPToSI:
      return PreparedI128RuntimeHelperKind::FloatToSignedInt;
    case bir::CastOpcode::FPToUI:
      return PreparedI128RuntimeHelperKind::FloatToUnsignedInt;
    case bir::CastOpcode::SIToFP:
      return PreparedI128RuntimeHelperKind::SignedIntToFloat;
    case bir::CastOpcode::UIToFP:
      return PreparedI128RuntimeHelperKind::UnsignedIntToFloat;
    case bir::CastOpcode::SExt:
    case bir::CastOpcode::ZExt:
    case bir::CastOpcode::Trunc:
    case bir::CastOpcode::FPTrunc:
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      break;
  }
  return PreparedI128RuntimeHelperKind::FloatToSignedInt;
}

[[nodiscard]] std::string_view i128_float_integer_conversion_helper_callee(
    const bir::CastInst& cast) {
  switch (cast.opcode) {
    case bir::CastOpcode::FPToSI:
      if (cast.operand.type == bir::TypeKind::F32 && cast.result.type == bir::TypeKind::I128) {
        return "__fixsfti";
      }
      if (cast.operand.type == bir::TypeKind::F64 && cast.result.type == bir::TypeKind::I128) {
        return "__fixdfti";
      }
      break;
    case bir::CastOpcode::FPToUI:
      if (cast.operand.type == bir::TypeKind::F32 && cast.result.type == bir::TypeKind::I128) {
        return "__fixunssfti";
      }
      if (cast.operand.type == bir::TypeKind::F64 && cast.result.type == bir::TypeKind::I128) {
        return "__fixunsdfti";
      }
      break;
    case bir::CastOpcode::SIToFP:
      if (cast.operand.type == bir::TypeKind::I128 && cast.result.type == bir::TypeKind::F32) {
        return "__floattisf";
      }
      if (cast.operand.type == bir::TypeKind::I128 && cast.result.type == bir::TypeKind::F64) {
        return "__floattidf";
      }
      break;
    case bir::CastOpcode::UIToFP:
      if (cast.operand.type == bir::TypeKind::I128 && cast.result.type == bir::TypeKind::F32) {
        return "__floatuntisf";
      }
      if (cast.operand.type == bir::TypeKind::I128 && cast.result.type == bir::TypeKind::F64) {
        return "__floatuntidf";
      }
      break;
    case bir::CastOpcode::SExt:
    case bir::CastOpcode::ZExt:
    case bir::CastOpcode::Trunc:
    case bir::CastOpcode::FPTrunc:
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      break;
  }
  return "";
}

void expire_completed_assignments(std::vector<ActiveRegisterAssignment>& active,
                                  std::size_t start_point) {
  active.erase(std::remove_if(active.begin(),
                              active.end(),
                              [start_point](const ActiveRegisterAssignment& assignment) {
                                return assignment.end_point < start_point;
                              }),
               active.end());
}

[[nodiscard]] PreparedMovePhase classify_prepared_move_phase(const PreparedMoveResolution& move) {
  switch (move.destination_kind) {
    case PreparedMoveDestinationKind::CallArgumentAbi:
      return PreparedMovePhase::BeforeCall;
    case PreparedMoveDestinationKind::CallResultAbi:
      return PreparedMovePhase::AfterCall;
    case PreparedMoveDestinationKind::FunctionReturnAbi:
      return PreparedMovePhase::BeforeReturn;
    case PreparedMoveDestinationKind::Value:
      return move.authority_kind == PreparedMoveAuthorityKind::OutOfSsaParallelCopy
                 ? PreparedMovePhase::BlockEntry
                 : PreparedMovePhase::BeforeInstruction;
  }
  return PreparedMovePhase::BeforeInstruction;
}

void append_prepared_move_bundle(PreparedValueLocationFunction& function_locations,
                                 const PreparedMoveResolution& move) {
  const PreparedMovePhase phase = classify_prepared_move_phase(move);
  const auto existing = std::find_if(
      function_locations.move_bundles.begin(),
      function_locations.move_bundles.end(),
      [&](const PreparedMoveBundle& bundle) {
        return bundle.phase == phase && bundle.authority_kind == move.authority_kind &&
               bundle.block_index == move.block_index &&
               bundle.instruction_index == move.instruction_index &&
               bundle.source_parallel_copy_predecessor_label ==
                   move.source_parallel_copy_predecessor_label &&
               bundle.source_parallel_copy_successor_label ==
                   move.source_parallel_copy_successor_label;
      });
  if (existing != function_locations.move_bundles.end()) {
    existing->moves.push_back(move);
    return;
  }
  function_locations.move_bundles.push_back(PreparedMoveBundle{
      .function_name = function_locations.function_name,
      .phase = phase,
      .authority_kind = move.authority_kind,
      .block_index = move.block_index,
      .instruction_index = move.instruction_index,
      .source_parallel_copy_predecessor_label = move.source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label = move.source_parallel_copy_successor_label,
      .moves = {move},
  });
}

void append_prepared_abi_binding(PreparedValueLocationFunction& function_locations,
                                 PreparedMovePhase phase,
                                 std::size_t block_index,
                                 std::size_t instruction_index,
                                 PreparedAbiBinding binding) {
  auto existing = std::find_if(
      function_locations.move_bundles.begin(),
      function_locations.move_bundles.end(),
      [&](const PreparedMoveBundle& bundle) {
        return bundle.phase == phase && bundle.block_index == block_index &&
               bundle.instruction_index == instruction_index;
      });
  if (existing == function_locations.move_bundles.end()) {
    function_locations.move_bundles.push_back(PreparedMoveBundle{
        .function_name = function_locations.function_name,
        .phase = phase,
        .block_index = block_index,
        .instruction_index = instruction_index,
        .moves = {},
        .abi_bindings = {},
    });
    existing = std::prev(function_locations.move_bundles.end());
  }

  const auto duplicate = std::find_if(
      existing->abi_bindings.begin(),
      existing->abi_bindings.end(),
      [&](const PreparedAbiBinding& existing_binding) {
        return existing_binding.destination_kind == binding.destination_kind &&
               existing_binding.destination_storage_kind == binding.destination_storage_kind &&
               existing_binding.destination_abi_index == binding.destination_abi_index &&
               existing_binding.destination_register_name == binding.destination_register_name &&
               existing_binding.destination_contiguous_width == binding.destination_contiguous_width &&
               existing_binding.destination_occupied_register_names ==
                   binding.destination_occupied_register_names &&
               existing_binding.destination_register_placement ==
                   binding.destination_register_placement &&
               existing_binding.destination_stack_offset_bytes ==
                   binding.destination_stack_offset_bytes;
      });
  if (duplicate != existing->abi_bindings.end()) {
    return;
  }
  existing->abi_bindings.push_back(std::move(binding));
}

void append_prepared_call_abi_bindings(const PreparedNameTables& names,
                                       const c4c::TargetProfile& target_profile,
                                       const c4c::backend::bir::Function& function,
                                       const PreparedRegallocFunction& regalloc_function,
                                       PreparedValueLocationFunction& function_locations) {
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
      if (call == nullptr) {
        continue;
      }

      for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
        const PreparedMoveStorageKind destination_storage_kind =
            call_arg_storage_kind(target_profile, *call, arg_index);
        if (destination_storage_kind == PreparedMoveStorageKind::None) {
          continue;
        }
        const auto& arg = call->args[arg_index];
        const auto arg_abi = resolve_call_arg_abi(target_profile, *call, arg_index);
        const auto abi_register_name =
            arg_abi.has_value()
                ? call_arg_destination_register_name(target_profile, *arg_abi, arg_index)
                : std::nullopt;
        const auto stack_offset = call_arg_destination_stack_offset_bytes(target_profile, *call, arg_index);
        const auto* source =
            arg.kind == bir::Value::Kind::Named
                ? find_regalloc_value(regalloc_function, names, arg.name)
                : nullptr;
        const std::size_t destination_contiguous_width =
            source != nullptr ? published_register_group_width(*source) : 1;
        const auto abi_register_placement =
            destination_storage_kind == PreparedMoveStorageKind::Register && arg_abi.has_value()
                ? f128_call_arg_destination_placement(
                      call_arg_destination_register_placement(target_profile,
                                                             *arg_abi,
                                                             arg_index,
                                                             destination_contiguous_width),
                      arg.type)
                : std::nullopt;
        const auto destination_occupied_register_names =
            destination_storage_kind == PreparedMoveStorageKind::Register &&
                    abi_register_name.has_value()
                ? call_arg_destination_register_names(target_profile,
                                                      source != nullptr ? source->register_class
                                                                         : PreparedRegisterClass::None,
                                                      arg_index,
                                                      *abi_register_name,
                                                      destination_contiguous_width)
                : std::vector<std::string>{};
        append_prepared_abi_binding(function_locations,
                                    PreparedMovePhase::BeforeCall,
                                    block_index,
                                    instruction_index,
                                    PreparedAbiBinding{
                                        .destination_kind = PreparedMoveDestinationKind::CallArgumentAbi,
                                        .destination_storage_kind = destination_storage_kind,
                                        .destination_abi_index = arg_index,
                                        .destination_register_name =
                                            destination_storage_kind == PreparedMoveStorageKind::Register
                                                ? abi_register_name
                                                : std::nullopt,
                                        .destination_contiguous_width =
                                            destination_storage_kind == PreparedMoveStorageKind::Register
                                                ? destination_contiguous_width
                                                : 1,
                                        .destination_occupied_register_names =
                                            destination_storage_kind == PreparedMoveStorageKind::Register
                                                ? destination_occupied_register_names
                                                : std::vector<std::string>{},
                                        .destination_stack_offset_bytes =
                                            destination_storage_kind == PreparedMoveStorageKind::StackSlot
                                                ? stack_offset
                                                : std::nullopt,
                                        .destination_register_placement =
                                            destination_storage_kind == PreparedMoveStorageKind::Register
                                                ? abi_register_placement
                                                : std::nullopt,
                                    });
        if (arg_abi.has_value() &&
            arg_abi->type == bir::TypeKind::Ptr &&
            arg_abi->byval_copy &&
            abi_register_name.has_value()) {
          if (destination_storage_kind == PreparedMoveStorageKind::Register &&
              stack_offset.has_value()) {
            append_prepared_abi_binding(
                function_locations,
                PreparedMovePhase::BeforeCall,
                block_index,
                instruction_index,
                PreparedAbiBinding{
                    .destination_kind = PreparedMoveDestinationKind::CallArgumentAbi,
                    .destination_storage_kind = PreparedMoveStorageKind::StackSlot,
                    .destination_abi_index = arg_index,
                    .destination_register_name = std::nullopt,
                    .destination_contiguous_width = 1,
                    .destination_occupied_register_names = {},
                    .destination_stack_offset_bytes = stack_offset,
                    .destination_register_placement = std::nullopt,
                });
          }
          const auto byval_register_placement =
              call_arg_destination_register_placement(target_profile, *arg_abi, arg_index, 1);
          append_prepared_abi_binding(function_locations,
                                      PreparedMovePhase::BeforeCall,
                                      block_index,
                                      instruction_index,
                                      PreparedAbiBinding{
                                          .destination_kind =
                                              PreparedMoveDestinationKind::CallArgumentAbi,
                                          .destination_storage_kind =
                                              PreparedMoveStorageKind::Register,
                                          .destination_abi_index = arg_index,
                                          .destination_register_name =
                                              abi_register_name,
                                          .destination_contiguous_width = 1,
                                          .destination_occupied_register_names =
                                              std::vector<std::string>{*abi_register_name},
                                          .destination_stack_offset_bytes = std::nullopt,
                                          .destination_register_placement =
                                              byval_register_placement,
                                      });
        }
      }

      const PreparedMoveStorageKind result_storage_kind = call_result_storage_kind(*call);
      if (result_storage_kind == PreparedMoveStorageKind::None) {
        continue;
      }
      const auto destination_register_name =
          call->result_abi.has_value()
              ? call_result_destination_register_name(target_profile, *call->result_abi)
              : std::nullopt;
      const auto* result =
          call->result.has_value() && call->result->kind == bir::Value::Kind::Named
              ? find_regalloc_value(regalloc_function, names, call->result->name)
              : nullptr;
      const std::size_t destination_contiguous_width =
          result != nullptr ? published_register_group_width(*result) : 1;
      const auto destination_register_placement =
          result_storage_kind == PreparedMoveStorageKind::Register &&
                  call->result_abi.has_value()
              ? call_result_destination_register_placement(target_profile,
                                                           *call->result_abi,
                                                           destination_contiguous_width)
              : std::nullopt;
      append_prepared_abi_binding(function_locations,
                                  PreparedMovePhase::AfterCall,
                                  block_index,
                                  instruction_index,
                                  PreparedAbiBinding{
                                      .destination_kind = PreparedMoveDestinationKind::CallResultAbi,
                                      .destination_storage_kind = result_storage_kind,
                                      .destination_abi_index = std::nullopt,
                                      .destination_register_name = destination_register_name,
                                      .destination_contiguous_width =
                                          result_storage_kind == PreparedMoveStorageKind::Register
                                              ? destination_contiguous_width
                                              : 1,
                                      .destination_occupied_register_names =
                                          result_storage_kind == PreparedMoveStorageKind::Register &&
                                                  destination_register_name.has_value()
                                              ? call_result_destination_register_names(
                                                    target_profile,
                                                    result != nullptr ? result->register_class
                                                                      : PreparedRegisterClass::None,
                                                    *destination_register_name,
                                                    destination_contiguous_width)
                                              : std::vector<std::string>{},
                                      .destination_stack_offset_bytes = std::nullopt,
                                      .destination_register_placement =
                                          destination_register_placement,
                                  });
    }
  }
}

[[nodiscard]] PreparedValueLocationFunction build_prepared_value_location_function(
    PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::bir::Function* function,
    const PreparedAddressingFunction* function_addressing,
    const PreparedRegallocFunction& regalloc_function) {
  PreparedValueLocationFunction function_locations{
      .function_name = regalloc_function.function_name,
      .value_homes = {},
      .move_bundles = {},
  };
  function_locations.value_homes = build_prepared_value_homes(names,
                                                             target_profile,
                                                             function,
                                                             function_addressing,
                                                             regalloc_function);
  for (const auto& move : regalloc_function.move_resolution) {
    append_prepared_move_bundle(function_locations, move);
  }
  if (function != nullptr) {
    append_prepared_call_abi_bindings(
        names, target_profile, *function, regalloc_function, function_locations);
  }
  return function_locations;
}

[[nodiscard]] const bir::Function* find_bir_function(const bir::Module& module,
                                                     const PreparedNameTables& names,
                                                     FunctionNameId function_name) {
  const std::string_view function_spelling = prepared_function_name(names, function_name);
  const auto it = std::find_if(module.functions.begin(),
                               module.functions.end(),
                               [function_spelling](const bir::Function& function) {
                                 return function.name == function_spelling;
                               });
  if (it == module.functions.end()) {
    return nullptr;
  }
  return &*it;
}

[[nodiscard]] const PreparedRegallocValue* find_f128_helper_operand_value(
    const PreparedRegallocFunction& function,
    const PreparedNameTables& names,
    const bir::Value& value) {
  if (value.kind == bir::Value::Kind::Named) {
    return find_regalloc_value(function, names, value.name);
  }
  return find_f128_constant_regalloc_value(function, value);
}

void append_i128_runtime_helper_fact(PreparedI128RuntimeHelperFunction& function_helpers,
                                     std::string fact) {
  if (std::find(function_helpers.missing_required_facts.begin(),
                function_helpers.missing_required_facts.end(),
                fact) == function_helpers.missing_required_facts.end()) {
    function_helpers.missing_required_facts.push_back(std::move(fact));
  }
}

void append_f128_runtime_helper_fact(PreparedF128RuntimeHelperFunction& function_helpers,
                                     std::string fact) {
  if (std::find(function_helpers.missing_required_facts.begin(),
                function_helpers.missing_required_facts.end(),
                fact) == function_helpers.missing_required_facts.end()) {
    function_helpers.missing_required_facts.push_back(std::move(fact));
  }
}

void append_f128_runtime_helper_mappings(const PreparedNameTables& names,
                                         const bir::Function& function,
                                         const PreparedRegallocFunction& regalloc_function,
                                         PreparedF128RuntimeHelpers& helper_mappings) {
  PreparedF128RuntimeHelperFunction function_helpers{
      .function_name = regalloc_function.function_name,
      .helpers = {},
      .missing_required_facts = {},
  };

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
         ++instruction_index) {
      const auto& inst = block.insts[instruction_index];
      const auto* binary = std::get_if<bir::BinaryInst>(&inst);
      if (binary != nullptr && binary->operand_type == bir::TypeKind::F128 &&
          is_f128_soft_float_helper_opcode(binary->opcode)) {
        const bool is_comparison = bir::is_compare_opcode(binary->opcode);
        const bir::TypeKind required_result_type =
            is_comparison ? bir::TypeKind::I1 : bir::TypeKind::F128;
        if (binary->result.type != required_result_type) {
          append_f128_runtime_helper_fact(
              function_helpers,
              "f128_soft_float_helper_requires_matching_result_type");
          continue;
        }

        if (binary->result.kind != bir::Value::Kind::Named) {
          append_f128_runtime_helper_fact(
              function_helpers,
              "f128_soft_float_helper_requires_named_result");
          continue;
        }
        const auto* result =
            find_regalloc_value(regalloc_function, names, binary->result.name);
        const auto* lhs =
            find_f128_helper_operand_value(regalloc_function, names, binary->lhs);
        const auto* rhs =
            find_f128_helper_operand_value(regalloc_function, names, binary->rhs);
        if (result == nullptr || lhs == nullptr || rhs == nullptr) {
          append_f128_runtime_helper_fact(
              function_helpers,
              "f128_soft_float_helper_requires_prepared_value_id_for_result_lhs_rhs");
          continue;
        }
        const auto callee = f128_soft_float_helper_callee(binary->opcode);
        if (callee.empty()) {
          append_f128_runtime_helper_fact(
              function_helpers,
              "f128_soft_float_helper_requires_callee_identity");
          continue;
        }

        function_helpers.helpers.push_back(PreparedF128RuntimeHelper{
            .function_name = regalloc_function.function_name,
            .block_index = block_index,
            .instruction_index = instruction_index,
            .source_binary_opcode = binary->opcode,
            .source_type = binary->operand_type,
            .result_type = is_comparison ? bir::TypeKind::I32 : binary->result.type,
            .result_value_id = result->value_id,
            .result_value_name = result->value_name,
            .lhs_value_id = lhs->value_id,
            .lhs_value_name = lhs->value_name,
            .rhs_value_id = rhs->value_id,
            .rhs_value_name = rhs->value_name,
            .helper_family = is_comparison
                                 ? PreparedF128RuntimeHelperFamily::Comparison
                                 : PreparedF128RuntimeHelperFamily::Arithmetic,
            .helper_kind = f128_soft_float_helper_kind(binary->opcode),
            .callee_name = std::string(callee),
            .result_ownership =
                is_comparison
                    ? PreparedF128RuntimeHelperResultOwnership::ScalarCmpResult
                    : PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier,
            .scalar_cmp_result_consumption =
                is_comparison
                    ? std::optional<PreparedF128RuntimeHelper::ScalarCmpResultConsumption>{
                          PreparedF128RuntimeHelper::ScalarCmpResultConsumption{
                              .cmp_type = bir::TypeKind::I32,
                              .bir_result_type = bir::TypeKind::I1,
                              .zero_test = f128_cmp_result_zero_test(binary->opcode),
                              .consumes_helper_cmp_result = true,
                              .owns_bir_i1_result = true,
                          }}
                    : std::nullopt,
        });
        continue;
      }

      const auto* cast = std::get_if<bir::CastInst>(&inst);
      if (cast == nullptr || !is_f128_float_width_conversion_cast(*cast)) {
        continue;
      }
      if (cast->result.kind != bir::Value::Kind::Named ||
          cast->operand.kind != bir::Value::Kind::Named) {
        append_f128_runtime_helper_fact(
            function_helpers,
            "f128_cast_helper_requires_named_result_and_operand");
        continue;
      }
      const auto* result =
          find_regalloc_value(regalloc_function, names, cast->result.name);
      const auto* operand =
          find_regalloc_value(regalloc_function, names, cast->operand.name);
      if (result == nullptr || operand == nullptr) {
        append_f128_runtime_helper_fact(
            function_helpers,
            "f128_cast_helper_requires_prepared_value_id_for_result_operand");
        continue;
      }
      const auto callee = f128_cast_helper_callee(*cast);
      if (callee.empty()) {
        append_f128_runtime_helper_fact(
            function_helpers,
            "f128_cast_helper_requires_callee_identity");
        continue;
      }
      const bool scalar_to_f128 = cast->result.type == bir::TypeKind::F128;
      function_helpers.helpers.push_back(PreparedF128RuntimeHelper{
          .function_name = regalloc_function.function_name,
          .block_index = block_index,
          .instruction_index = instruction_index,
          .source_cast_opcode = cast->opcode,
          .source_type = cast->operand.type,
          .result_type = cast->result.type,
          .result_value_id = result->value_id,
          .result_value_name = result->value_name,
          .operand_value_id = operand->value_id,
          .operand_value_name = operand->value_name,
          .lhs_value_id = scalar_to_f128 ? result->value_id : operand->value_id,
          .lhs_value_name = scalar_to_f128 ? result->value_name : operand->value_name,
          .helper_family = PreparedF128RuntimeHelperFamily::Cast,
          .helper_kind = f128_cast_helper_kind(*cast),
          .callee_name = std::string(callee),
          .result_ownership =
              scalar_to_f128
                  ? PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier
                  : PreparedF128RuntimeHelperResultOwnership::ScalarValue,
      });
    }
  }

  if (!function_helpers.helpers.empty() ||
      !function_helpers.missing_required_facts.empty()) {
    helper_mappings.functions.push_back(std::move(function_helpers));
  }
}

void append_i128_runtime_helper_mappings(const PreparedNameTables& names,
                                         const bir::Function& function,
                                         const PreparedRegallocFunction& regalloc_function,
                                         PreparedI128RuntimeHelpers& helper_mappings) {
  PreparedI128RuntimeHelperFunction function_helpers{
      .function_name = regalloc_function.function_name,
      .helpers = {},
      .missing_required_facts = {},
  };

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
         ++instruction_index) {
      const auto& inst = block.insts[instruction_index];
      if (const auto* binary = std::get_if<bir::BinaryInst>(&inst);
          binary != nullptr && binary->operand_type == bir::TypeKind::I128 &&
          binary->result.type == bir::TypeKind::I128 &&
          is_i128_div_rem_helper_opcode(binary->opcode)) {
        if (binary->result.kind != bir::Value::Kind::Named ||
            binary->lhs.kind != bir::Value::Kind::Named ||
            binary->rhs.kind != bir::Value::Kind::Named) {
          append_i128_runtime_helper_fact(
              function_helpers,
              "i128_div_rem_helper_requires_named_result_and_operands");
          continue;
        }
        const auto* result =
            find_regalloc_value(regalloc_function, names, binary->result.name);
        const auto* lhs = find_regalloc_value(regalloc_function, names, binary->lhs.name);
        const auto* rhs = find_regalloc_value(regalloc_function, names, binary->rhs.name);
        if (result == nullptr || lhs == nullptr || rhs == nullptr) {
          append_i128_runtime_helper_fact(
              function_helpers,
              "i128_div_rem_helper_requires_prepared_value_id_for_result_lhs_rhs");
          continue;
        }
        PreparedI128RuntimeHelper helper{
            .function_name = regalloc_function.function_name,
            .block_index = block_index,
            .instruction_index = instruction_index,
            .source_binary_opcode = binary->opcode,
            .source_type = binary->operand_type,
            .result_type = binary->result.type,
            .result_value_id = result->value_id,
            .result_value_name = result->value_name,
            .lhs_value_id = lhs->value_id,
            .lhs_value_name = lhs->value_name,
            .rhs_value_id = rhs->value_id,
            .rhs_value_name = rhs->value_name,
            .helper_family = PreparedI128RuntimeHelperFamily::DivRem,
            .helper_kind = i128_div_rem_helper_kind(binary->opcode),
            .callee_name = std::string(i128_div_rem_helper_callee(binary->opcode)),
            .result_ownership =
                PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes,
        };
        function_helpers.helpers.push_back(std::move(helper));
        continue;
      }

      if (const auto* cast = std::get_if<bir::CastInst>(&inst);
          cast != nullptr && is_i128_float_integer_conversion_cast(*cast)) {
        if (!is_supported_i128_float_integer_conversion_cast(*cast)) {
          append_i128_runtime_helper_fact(
              function_helpers,
              "i128_float_integer_conversion_helper_mapping_deferred");
          continue;
        }
        if (cast->result.kind != bir::Value::Kind::Named ||
            cast->operand.kind != bir::Value::Kind::Named) {
          append_i128_runtime_helper_fact(
              function_helpers,
              "i128_float_integer_conversion_helper_requires_named_result_and_operand");
          continue;
        }
        const auto* result =
            find_regalloc_value(regalloc_function, names, cast->result.name);
        const auto* operand =
            find_regalloc_value(regalloc_function, names, cast->operand.name);
        if (result == nullptr || operand == nullptr) {
          append_i128_runtime_helper_fact(
              function_helpers,
              "i128_float_integer_conversion_helper_requires_prepared_value_id_for_result_operand");
          continue;
        }
        const auto callee = i128_float_integer_conversion_helper_callee(*cast);
        if (callee.empty()) {
          append_i128_runtime_helper_fact(
              function_helpers,
              "i128_float_integer_conversion_helper_requires_callee_identity");
          continue;
        }
        const bool source_signed = cast->opcode == bir::CastOpcode::SIToFP;
        const bool result_signed = cast->opcode == bir::CastOpcode::FPToSI;
        PreparedI128RuntimeHelper helper{
            .function_name = regalloc_function.function_name,
            .block_index = block_index,
            .instruction_index = instruction_index,
            .source_binary_opcode = bir::BinaryOpcode::Add,
            .source_cast_opcode = cast->opcode,
            .source_type = cast->operand.type,
            .result_type = cast->result.type,
            .source_width_bytes = i128_helper_type_width_bytes(cast->operand.type),
            .result_width_bytes = i128_helper_type_width_bytes(cast->result.type),
            .source_signed = source_signed,
            .result_signed = result_signed,
            .result_value_id = result->value_id,
            .result_value_name = result->value_name,
            .operand_value_id = operand->value_id,
            .operand_value_name = operand->value_name,
            .lhs_value_id = operand->value_id,
            .lhs_value_name = operand->value_name,
            .rhs_value_id = 0,
            .rhs_value_name = kInvalidValueName,
            .helper_family = PreparedI128RuntimeHelperFamily::FloatIntegerConversion,
            .helper_kind = i128_float_integer_conversion_helper_kind(cast->opcode),
            .callee_name = std::string(callee),
            .result_ownership =
                cast->result.type == bir::TypeKind::I128
                    ? PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes
                    : PreparedI128RuntimeHelperResultOwnership::ScalarValue,
        };
        function_helpers.helpers.push_back(std::move(helper));
      }
    }
  }

  if (!function_helpers.helpers.empty() ||
      !function_helpers.missing_required_facts.empty()) {
    helper_mappings.functions.push_back(std::move(function_helpers));
  }
}

[[nodiscard]] std::optional<std::size_t> find_instruction_index_for_named_result(
    const bir::Block& block,
    std::string_view value_name) {
  for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
    bool matches_result = std::visit(
        [&](const auto& inst) {
          using Inst = std::decay_t<decltype(inst)>;
          if constexpr (std::is_same_v<Inst, bir::BinaryInst> || std::is_same_v<Inst, bir::SelectInst> ||
                        std::is_same_v<Inst, bir::CastInst> || std::is_same_v<Inst, bir::LoadLocalInst> ||
                        std::is_same_v<Inst, bir::LoadGlobalInst>) {
            return inst.result.kind == bir::Value::Kind::Named && inst.result.name == value_name;
          } else if constexpr (std::is_same_v<Inst, bir::CallInst>) {
            return inst.result.has_value() && inst.result->kind == bir::Value::Kind::Named &&
                   inst.result->name == value_name;
          } else {
            return false;
          }
        },
        block.insts[instruction_index]);
    if (matches_result) {
      return instruction_index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<BlockLabelId> resolve_existing_consumer_block_label_id(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const bir::Block& block) {
  if (block.label_id != kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(block.label_id);
    if (!structured_label.empty()) {
      const BlockLabelId prepared_label_id = names.block_labels.find(structured_label);
      if (prepared_label_id != kInvalidBlockLabel) {
        return prepared_label_id;
      }
    }
  }
  return resolve_prepared_block_label_id(names, block.label);
}

void append_consumer_move_resolution(const PreparedNameTables& names,
                                     const bir::NameTables& bir_names,
                                     const bir::Function& function,
                                     const PreparedControlFlowFunction* function_cf,
                                     PreparedRegallocFunction& regalloc_function) {
  auto is_authoritative_select_materialized_join_result =
      [&](const bir::Block& block, const bir::SelectInst& select) {
        if (function_cf == nullptr || select.result.kind != bir::Value::Kind::Named) {
          return false;
        }

        const auto block_label_id = resolve_existing_consumer_block_label_id(names, bir_names, block);
        if (!block_label_id.has_value()) {
          return false;
        }

        const auto* join_transfer =
            find_prepared_join_transfer(names, *function_cf, *block_label_id, select.result.name);
        return join_transfer != nullptr &&
               effective_prepared_join_transfer_carrier_kind(*join_transfer) ==
                   PreparedJoinTransferCarrierKind::SelectMaterialization;
      };

  auto append_for_instruction = [&](const bir::Value& result,
                                    std::initializer_list<const bir::Value*> operands,
                                    std::size_t block_index,
                                    std::size_t instruction_index) {
    if (result.kind != bir::Value::Kind::Named) {
      return;
    }

    const auto* destination = find_regalloc_value(regalloc_function, names, result.name);
    if (destination == nullptr) {
      return;
    }

    for (const bir::Value* operand : operands) {
      if (operand == nullptr || operand->kind != bir::Value::Kind::Named) {
        continue;
      }
      const auto* source = find_regalloc_value(regalloc_function, names, operand->name);
      if (source == nullptr) {
        continue;
      }

      append_move_resolution_record(regalloc_function,
                                    *source,
                                    *destination,
                                    block_index,
                                    instruction_index,
                                    false,
                                    false,
                                    std::nullopt,
                                    PreparedMoveResolutionOpKind::Move,
                                    PreparedMoveAuthorityKind::None,
                                    storage_transfer_reason("consumer", *source, *destination));
    }
  };

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      std::visit(
          [&](const auto& inst) {
            using Inst = std::decay_t<decltype(inst)>;
            if constexpr (std::is_same_v<Inst, bir::BinaryInst>) {
              append_for_instruction(inst.result,
                                     {&inst.lhs, &inst.rhs},
                                     block_index,
                                     instruction_index);
            } else if constexpr (std::is_same_v<Inst, bir::SelectInst>) {
              // Select-materialized joins are already owned by published out-of-SSA
              // join-transfer authority. Re-adding them here would regress into
              // consumer-shaped reconstruction.
              if (is_authoritative_select_materialized_join_result(block, inst)) {
                return;
              }
              append_for_instruction(inst.result,
                                     {&inst.lhs, &inst.rhs, &inst.true_value, &inst.false_value},
                                     block_index,
                                     instruction_index);
            } else if constexpr (std::is_same_v<Inst, bir::CastInst>) {
              append_for_instruction(inst.result, {&inst.operand}, block_index, instruction_index);
            }
          },
          block.insts[instruction_index]);
    }
  }
}

void append_spill_reload_ops(const PreparedLivenessFunction& liveness_function,
                             const std::vector<std::optional<std::size_t>>& spill_points,
                             PreparedRegallocFunction& regalloc_function) {
  for (std::size_t value_index = 0; value_index < regalloc_function.values.size(); ++value_index) {
    const auto spill_point = spill_points[value_index];
    const auto& value = regalloc_function.values[value_index];
    if (!spill_point.has_value() || !value.assigned_stack_slot.has_value()) {
      continue;
    }
    const auto& published_register = value.assigned_register.has_value()
                                         ? value.assigned_register
                                         : value.spill_register_authority;

    if (const auto spill_location = locate_program_point(liveness_function, *spill_point);
        spill_location.has_value()) {
      regalloc_function.spill_reload_ops.push_back(PreparedSpillReloadOp{
          .value_id = value.value_id,
          .op_kind = PreparedSpillReloadOpKind::Spill,
          .block_index = spill_location->block_index,
          .instruction_index = spill_location->instruction_index,
          .register_bank = register_bank_from_class(value.register_class),
          .register_name = published_register.has_value()
                               ? std::optional<std::string>{published_register->register_name}
                               : std::nullopt,
          .contiguous_width = published_register.has_value()
                                  ? published_register->contiguous_width
                                  : std::max<std::size_t>(value.register_group_width, 1),
          .occupied_register_names = published_register.has_value()
                                         ? published_register->occupied_register_names
                                         : std::vector<std::string>{},
          .slot_id = value.assigned_stack_slot.has_value()
                         ? std::optional<PreparedFrameSlotId>{value.assigned_stack_slot->slot_id}
                         : std::nullopt,
          .stack_offset_bytes = value.assigned_stack_slot.has_value()
                                    ? std::optional<std::size_t>{
                                          value.assigned_stack_slot->offset_bytes}
                                    : std::nullopt,
      });
    }

    std::optional<std::size_t> last_reload_point;
    for (const std::size_t use_point : liveness_function.values[value_index].use_points) {
      if (use_point <= *spill_point || last_reload_point == use_point) {
        continue;
      }
      if (const auto reload_location = locate_program_point(liveness_function, use_point);
          reload_location.has_value()) {
        regalloc_function.spill_reload_ops.push_back(PreparedSpillReloadOp{
            .value_id = value.value_id,
            .op_kind = PreparedSpillReloadOpKind::Reload,
            .block_index = reload_location->block_index,
            .instruction_index = reload_location->instruction_index,
            .register_bank = register_bank_from_class(value.register_class),
            .register_name = published_register.has_value()
                                 ? std::optional<std::string>{published_register->register_name}
                                 : std::nullopt,
            .contiguous_width = published_register.has_value()
                                    ? published_register->contiguous_width
                                    : std::max<std::size_t>(value.register_group_width, 1),
            .occupied_register_names = published_register.has_value()
                                           ? published_register->occupied_register_names
                                           : std::vector<std::string>{},
            .slot_id = value.assigned_stack_slot.has_value()
                           ? std::optional<PreparedFrameSlotId>{value.assigned_stack_slot->slot_id}
                           : std::nullopt,
            .stack_offset_bytes = value.assigned_stack_slot.has_value()
                                      ? std::optional<std::size_t>{
                                            value.assigned_stack_slot->offset_bytes}
                                      : std::nullopt,
        });
        last_reload_point = use_point;
      }
    }
  }
}

}  // namespace

void BirPreAlloc::run_regalloc() {
  prepared_.completed_phases.push_back("regalloc");
  prepared_.regalloc.functions.clear();
  prepared_.regalloc.functions.reserve(prepared_.liveness.functions.size());
  prepared_.value_locations.functions.clear();
  prepared_.value_locations.functions.reserve(prepared_.liveness.functions.size());
  prepared_.i128_runtime_helpers.functions.clear();

  PreparedValueId next_synthetic_value_id = 0;
  for (const auto& liveness_function : prepared_.liveness.functions) {
    for (const auto& value : liveness_function.values) {
      next_synthetic_value_id = std::max(next_synthetic_value_id, value.value_id + 1U);
    }
  }

  for (const auto& liveness_function : prepared_.liveness.functions) {
    PreparedRegallocFunction regalloc_function{
        .function_name = liveness_function.function_name,
        .values = {},
        .constraints = {},
        .interference = {},
        .move_resolution = {},
        .spill_reload_ops = {},
    };
    regalloc_function.values.reserve(liveness_function.values.size());
    regalloc_function.constraints.reserve(liveness_function.values.size());
    const auto* function =
        find_bir_function(prepared_.module, prepared_.names, liveness_function.function_name);

    for (const auto& liveness_value : liveness_function.values) {
      const PreparedRegisterClass register_class = resolve_register_class(prepared_, liveness_value);
      const std::size_t register_group_width =
          resolve_register_group_width(prepared_, liveness_value);
      const bool eligible_for_register_seed =
          register_class != PreparedRegisterClass::None &&
          liveness_value.value_kind != PreparedValueKind::StackObject;
      const std::size_t base_priority = value_priority(liveness_value);
      const std::size_t weighted_use_priority = weighted_use_score(liveness_function, liveness_value);
      const std::size_t priority = weighted_use_priority +
                                   (base_priority >= liveness_value.use_points.size()
                                        ? base_priority - liveness_value.use_points.size()
                                        : 0U);
      const auto caller_saved_names = caller_saved_registers(prepared_.target_profile, register_class);
      const auto callee_saved_names = callee_saved_registers(prepared_.target_profile, register_class);
      const auto caller_saved_spans =
          caller_saved_register_spans(prepared_.target_profile, register_class, register_group_width);
      const auto callee_saved_spans =
          callee_saved_register_spans(prepared_.target_profile, register_class, register_group_width);

      regalloc_function.values.push_back(PreparedRegallocValue{
          .value_id = liveness_value.value_id,
          .stack_object_id = liveness_value.stack_object_id,
          .function_name = liveness_value.function_name,
          .value_name = liveness_value.value_name,
          .type = liveness_value.type,
          .constant_f128_payload = std::nullopt,
          .value_kind = liveness_value.value_kind,
          .register_class = register_class,
          .register_group_width = register_group_width,
          .allocation_status = PreparedAllocationStatus::Unallocated,
          .spillable = eligible_for_register_seed && !liveness_value.requires_home_slot,
          .requires_home_slot = liveness_value.requires_home_slot,
          .crosses_call = liveness_value.crosses_call,
          .priority = priority,
          .spill_weight = static_cast<double>(priority),
          .live_interval = liveness_value.live_interval,
          .assigned_register = std::nullopt,
          .assigned_stack_slot = std::nullopt,
          .spill_register_authority = std::nullopt,
      });

      if (!eligible_for_register_seed) {
        continue;
      }

      regalloc_function.constraints.push_back(PreparedAllocationConstraint{
          .value_id = liveness_value.value_id,
          .register_class = register_class,
          .register_group_width = register_group_width,
          .requires_register = !liveness_value.requires_home_slot,
          .requires_home_slot = liveness_value.requires_home_slot,
          .cannot_cross_call = liveness_value.crosses_call &&
                               register_class != PreparedRegisterClass::Float,
          .fixed_register_name = std::nullopt,
          .preferred_register_names = materialize_register_names(liveness_value.crosses_call
                                                                    ? callee_saved_names
                                                                    : caller_saved_names),
          .forbidden_register_names = materialize_register_names(liveness_value.crosses_call
                                                                     ? caller_saved_names
                                                                     : std::vector<std::string_view>{}),
          .fixed_register_placement = std::nullopt,
          .preferred_register_placements =
              materialize_register_placements(liveness_value.crosses_call ? callee_saved_spans
                                                                           : caller_saved_spans),
          .forbidden_register_placements =
              liveness_value.crosses_call
                  ? materialize_register_placements(caller_saved_spans)
                  : std::vector<PreparedRegisterPlacement>{},
      });
    }

    append_f128_constant_values_for_function(regalloc_function.values,
                                             prepared_.names,
                                             function,
                                             liveness_function.function_name,
                                             next_synthetic_value_id);

    for (std::size_t lhs_index = 0; lhs_index < regalloc_function.values.size(); ++lhs_index) {
      const auto& lhs = regalloc_function.values[lhs_index];
      if (!lhs.live_interval.has_value() || lhs.register_class == PreparedRegisterClass::None) {
        continue;
      }
      for (std::size_t rhs_index = lhs_index + 1U; rhs_index < regalloc_function.values.size(); ++rhs_index) {
        const auto& rhs = regalloc_function.values[rhs_index];
        if (!rhs.live_interval.has_value() || rhs.register_class == PreparedRegisterClass::None) {
          continue;
        }
        if (!intervals_overlap(*lhs.live_interval, *rhs.live_interval)) {
          continue;
        }
        regalloc_function.interference.push_back(PreparedInterferenceEdge{
            .lhs_value_id = lhs.value_id,
            .rhs_value_id = rhs.value_id,
            .reason = "overlapping_live_intervals",
        });
      }
    }

    PreparedFrameSlotId next_slot_id = 0;
    std::size_t next_offset_bytes = prepared_.stack_layout.frame_size_bytes;
    std::size_t frame_alignment_bytes = prepared_.stack_layout.frame_alignment_bytes;
    for (const auto& slot : prepared_.stack_layout.frame_slots) {
      next_slot_id = std::max(next_slot_id, slot.slot_id + 1U);
      next_offset_bytes = std::max(next_offset_bytes, slot.offset_bytes + slot.size_bytes);
      frame_alignment_bytes = std::max(frame_alignment_bytes, slot.align_bytes);
    }

    std::vector<std::size_t> allocation_order(regalloc_function.values.size());
    for (std::size_t index = 0; index < allocation_order.size(); ++index) {
      allocation_order[index] = index;
    }
    std::sort(allocation_order.begin(),
              allocation_order.end(),
              [&regalloc_function](std::size_t lhs_index, std::size_t rhs_index) {
                const auto& lhs = regalloc_function.values[lhs_index];
                const auto& rhs = regalloc_function.values[rhs_index];
                const std::size_t lhs_start = interval_start_sort_key(lhs);
                const std::size_t rhs_start = interval_start_sort_key(rhs);
                if (lhs_start != rhs_start) {
                  return lhs_start < rhs_start;
                }
                if (lhs.priority != rhs.priority) {
                  return lhs.priority > rhs.priority;
                }
                return lhs.value_id < rhs.value_id;
              });

    std::vector<ActiveRegisterAssignment> active_caller_saved_assignments;
    active_caller_saved_assignments.reserve(regalloc_function.values.size());
    std::vector<ActiveRegisterAssignment> active_callee_saved_assignments;
    active_callee_saved_assignments.reserve(regalloc_function.values.size());
    std::vector<std::optional<std::size_t>> spill_points(regalloc_function.values.size(), std::nullopt);
    std::size_t assigned_register_count = 0;
    std::size_t assigned_stack_count = 0;

    auto assign_from_pool = [&](const auto& should_consider,
                                const auto& register_pool_for_value,
                                std::vector<ActiveRegisterAssignment>& active_assignments,
                                const auto& can_evict_assignment) {
      for (const std::size_t value_index : allocation_order) {
        auto& value = regalloc_function.values[value_index];
        if (value.assigned_register.has_value() || value.assigned_stack_slot.has_value() ||
            value.requires_home_slot || !value.live_interval.has_value() ||
            value.register_class == PreparedRegisterClass::None || !should_consider(value)) {
          continue;
        }

        expire_completed_assignments(active_assignments, value.live_interval->start_point);
        const auto candidate_spans = register_pool_for_value(value);
        if (const auto chosen_register = choose_register_span(active_assignments, candidate_spans);
            chosen_register.has_value()) {
          value.assigned_register = PreparedPhysicalRegisterAssignment{
              .reg_class = value.register_class,
              .register_name = chosen_register->register_name,
              .contiguous_width = chosen_register->contiguous_width,
              .occupied_register_names = chosen_register->occupied_register_names,
          };
          value.allocation_status = PreparedAllocationStatus::AssignedRegister;
          active_assignments.push_back(ActiveRegisterAssignment{
              .value_index = value_index,
              .end_point = value.live_interval->end_point,
              .register_name = value.assigned_register->register_name,
              .occupied_register_names = value.assigned_register->occupied_register_names,
          });
          ++assigned_register_count;
          continue;
        }

        const auto eviction_index = choose_eviction_candidate(regalloc_function,
                                                              active_assignments,
                                                              candidate_spans,
                                                              value,
                                                              can_evict_assignment);
        if (!eviction_index.has_value()) {
          continue;
        }

        auto& evicted_value = regalloc_function.values[active_assignments[*eviction_index].value_index];
        spill_points[active_assignments[*eviction_index].value_index] = value.live_interval->start_point;
        value.assigned_register = PreparedPhysicalRegisterAssignment{
            .reg_class = value.register_class,
            .register_name = active_assignments[*eviction_index].register_name,
            .contiguous_width =
                regalloc_function.values[active_assignments[*eviction_index].value_index]
                    .assigned_register->contiguous_width,
            .occupied_register_names =
                regalloc_function.values[active_assignments[*eviction_index].value_index]
                    .assigned_register->occupied_register_names,
        };
        value.allocation_status = PreparedAllocationStatus::AssignedRegister;

        evicted_value.spill_register_authority = evicted_value.assigned_register;
        evicted_value.assigned_register.reset();
        evicted_value.allocation_status = PreparedAllocationStatus::Unallocated;

        active_assignments[*eviction_index] = ActiveRegisterAssignment{
            .value_index = value_index,
            .end_point = value.live_interval->end_point,
            .register_name = value.assigned_register->register_name,
            .occupied_register_names = value.assigned_register->occupied_register_names,
        };
      }
    };

    assign_from_pool([](const PreparedRegallocValue& value) { return value.crosses_call; },
                     [this](const PreparedRegallocValue& value) {
                       return callee_saved_register_spans(prepared_.target_profile,
                                                          value.register_class,
                                                          value.register_group_width);
                     },
                     active_callee_saved_assignments,
                     [](const ActiveRegisterAssignment&) { return true; });
    assign_from_pool([](const PreparedRegallocValue& value) { return !value.crosses_call; },
                     [this](const PreparedRegallocValue& value) {
                       return caller_saved_register_spans(prepared_.target_profile,
                                                          value.register_class,
                                                          value.register_group_width);
                     },
                     active_caller_saved_assignments,
                     [](const ActiveRegisterAssignment&) { return true; });
    assign_from_pool([](const PreparedRegallocValue& value) { return !value.crosses_call; },
                     [this](const PreparedRegallocValue& value) {
                       return callee_saved_register_spans(prepared_.target_profile,
                                                          value.register_class,
                                                          value.register_group_width);
                     },
                     active_callee_saved_assignments,
                     [&regalloc_function](const ActiveRegisterAssignment& assignment) {
                       return !regalloc_function.values[assignment.value_index].crosses_call;
                     });

    for (const std::size_t value_index : allocation_order) {
      auto& value = regalloc_function.values[value_index];

      if (value.assigned_register.has_value() || value.assigned_stack_slot.has_value()) {
        continue;
      }

      if (value.requires_home_slot) {
        value.assigned_stack_slot = allocate_stack_slot(value,
                                                        prepared_.stack_layout,
                                                        next_slot_id,
                                                        next_offset_bytes,
                                                        frame_alignment_bytes);
        value.allocation_status = PreparedAllocationStatus::AssignedStackSlot;
        ++assigned_stack_count;
        continue;
      }

      if (!value.live_interval.has_value() || value.register_class == PreparedRegisterClass::None) {
        if (normalized_value_size(value) != 0U) {
          value.assigned_stack_slot = allocate_stack_slot(value,
                                                          prepared_.stack_layout,
                                                          next_slot_id,
                                                          next_offset_bytes,
                                                          frame_alignment_bytes);
          value.allocation_status = PreparedAllocationStatus::AssignedStackSlot;
          ++assigned_stack_count;
        }
        continue;
      }

      value.assigned_stack_slot = allocate_stack_slot(value,
                                                      prepared_.stack_layout,
                                                      next_slot_id,
                                                      next_offset_bytes,
                                                      frame_alignment_bytes);
      value.allocation_status = PreparedAllocationStatus::AssignedStackSlot;
      ++assigned_stack_count;
    }

    append_spill_reload_ops(liveness_function, spill_points, regalloc_function);
    if (function != nullptr) {
      if (const auto* function_cf =
              find_prepared_control_flow_function(prepared_.control_flow, regalloc_function.function_name);
          function_cf != nullptr) {
        append_phi_move_resolution(prepared_.names, *function, *function_cf, regalloc_function);
        append_consumer_move_resolution(
            prepared_.names, prepared_.module.names, *function, function_cf, regalloc_function);
      } else {
        append_consumer_move_resolution(
            prepared_.names, prepared_.module.names, *function, nullptr, regalloc_function);
      }
      append_call_arg_move_resolution(
          prepared_.names, prepared_.target_profile, *function, regalloc_function);
      append_call_result_move_resolution(
          prepared_.names, prepared_.target_profile, *function, regalloc_function);
      append_return_move_resolution(
          prepared_.names, prepared_.target_profile, *function, regalloc_function);
      append_i128_runtime_helper_mappings(prepared_.names,
                                          *function,
                                          regalloc_function,
                                          prepared_.i128_runtime_helpers);
      append_f128_runtime_helper_mappings(prepared_.names,
                                          *function,
                                          regalloc_function,
                                          prepared_.f128_runtime_helpers);
    }

    prepared_.stack_layout.frame_size_bytes =
        std::max(prepared_.stack_layout.frame_size_bytes,
                 stack_layout::align_to(next_offset_bytes, std::max<std::size_t>(frame_alignment_bytes, 1U)));
    prepared_.stack_layout.frame_alignment_bytes =
        std::max(prepared_.stack_layout.frame_alignment_bytes, frame_alignment_bytes);

    prepared_.notes.push_back(PrepareNote{
        .phase = "regalloc",
        .message = "regalloc seeded function '" +
                   std::string(prepared_function_name(prepared_.names, regalloc_function.function_name)) +
                   "' with " +
                   std::to_string(regalloc_function.constraints.size()) +
                   " allocation constraint(s) and " +
                   std::to_string(regalloc_function.interference.size()) +
                   " interference edge(s) from active liveness; assigned " +
                   std::to_string(assigned_register_count) + " register(s) and " +
                   std::to_string(assigned_stack_count) + " stack slot(s)",
    });
    prepared_.value_locations.functions.push_back(build_prepared_value_location_function(
        prepared_.names,
        prepared_.target_profile,
        function,
        find_prepared_addressing_function(prepared_.addressing, regalloc_function.function_name),
        regalloc_function));
    prepared_.regalloc.functions.push_back(std::move(regalloc_function));
  }
}

}  // namespace c4c::backend::prepare
