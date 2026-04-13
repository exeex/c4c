#include "x86/codegen/x86_codegen.hpp"

namespace c4c::backend::x86 {

namespace {

bool is_float_type(IrType ty) {
  return ty == IrType::F32 || ty == IrType::F64;
}

bool is_scalar_gp_type(IrType ty) {
  switch (ty) {
    case IrType::I8:
    case IrType::I16:
    case IrType::I32:
    case IrType::I64:
    case IrType::I128:
    case IrType::U16:
    case IrType::U32:
    case IrType::U64:
    case IrType::Ptr:
      return true;
    default:
      return false;
  }
}

}  // namespace

ParamClassification classify_params_full(const IrFunction& func, const CallAbiConfig& config) {
  ParamClassification classification;
  classification.classes.reserve(func.params.size());

  std::size_t next_gp_reg = 0;
  std::size_t next_fp_reg = 0;
  std::int64_t next_stack_offset = 0;

  for (const auto& param : func.params) {
    ParamClassInfo info;
    if (is_float_type(param.type) && next_fp_reg < config.max_float_regs) {
      info.data = ParamClass::FloatReg{next_fp_reg++};
    } else if (is_scalar_gp_type(param.type) && next_gp_reg < config.max_int_regs) {
      info.data = ParamClass::IntReg{next_gp_reg++};
    } else {
      info.data = ParamClass::StackScalar{next_stack_offset};
      next_stack_offset += 8;
    }
    classification.classes.push_back(info);
  }

  return classification;
}

std::int64_t named_params_stack_bytes(const std::vector<ParamClassInfo>& classes) {
  std::int64_t total = 0;
  for (const auto& klass : classes) {
    if (std::holds_alternative<ParamClass::StackScalar>(klass.data)) {
      total += 8;
    } else if (const auto* stack = std::get_if<ParamClass::StructStack>(&klass.data)) {
      total += static_cast<std::int64_t>(stack->size);
    } else if (const auto* stack = std::get_if<ParamClass::LargeStructStack>(&klass.data)) {
      total += static_cast<std::int64_t>(stack->size);
    }
  }
  return total;
}

}  // namespace c4c::backend::x86
