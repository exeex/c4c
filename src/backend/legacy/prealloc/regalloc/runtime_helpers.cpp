#include "runtime_helpers.hpp"

#include "storage.hpp"
#include "values.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace c4c::backend::prepare::regalloc_detail {

namespace {
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

}  // namespace

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

}  // namespace c4c::backend::prepare::regalloc_detail
