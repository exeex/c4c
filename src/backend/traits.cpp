#include "riscv/codegen/riscv_codegen.hpp"

namespace c4c::backend {

namespace {

using c4c::backend::riscv::codegen::IrType;

constexpr bool is_i128_type(IrType type) { return type == IrType::I128; }

constexpr bool is_float_type(IrType type) {
  return type == IrType::F32 || type == IrType::F64 || type == IrType::F128;
}

constexpr bool is_signed_int_type(IrType type) {
  return type == IrType::I8 || type == IrType::I16 || type == IrType::I32 ||
         type == IrType::I64 || type == IrType::I128;
}

constexpr std::size_t type_size_bytes(IrType type) {
  switch (type) {
    case IrType::Void: return 0;
    case IrType::I8: return 1;
    case IrType::I16:
    case IrType::U16: return 2;
    case IrType::I32:
    case IrType::U32:
    case IrType::F32: return 4;
    case IrType::I64:
    case IrType::U64:
    case IrType::Ptr:
    case IrType::F64: return 8;
    case IrType::I128:
    case IrType::F128: return 16;
  }
  return 0;
}

constexpr bool is_long_double_type(IrType type) { return type == IrType::F128; }

}  // namespace

void emit_riscv_return_default(
    riscv::codegen::RiscvCodegen& codegen,
    const riscv::codegen::Operand* value,
    std::int64_t frame_size) {
  if (value != nullptr) {
    const auto return_type = codegen.current_return_type_impl();
    if (is_i128_type(return_type)) {
      codegen.emit_load_acc_pair_for_return_default(*value);
      codegen.emit_return_i128_to_regs_impl();
      codegen.emit_epilogue_and_ret_impl(frame_size);
      return;
    }

    codegen.emit_load_operand_for_return_default(*value);
    if (is_long_double_type(return_type)) {
      codegen.emit_return_f128_to_reg_impl();
    } else if (return_type == riscv::codegen::IrType::F32) {
      codegen.emit_return_f32_to_reg_impl();
    } else if (is_float_type(return_type)) {
      codegen.emit_return_f64_to_reg_impl();
    } else {
      codegen.emit_return_int_to_reg_impl();
    }
  }

  codegen.emit_epilogue_and_ret_impl(frame_size);
}

void emit_cast_default(riscv::codegen::RiscvCodegen& codegen,
                       const riscv::codegen::Value& dest,
                       const riscv::codegen::Operand& src,
                       riscv::codegen::IrType from_ty,
                       riscv::codegen::IrType to_ty) {
  if (is_i128_type(to_ty) && is_float_type(from_ty) && !is_long_double_type(from_ty)) {
    codegen.emit_float_to_i128_call_impl(src, is_signed_int_type(to_ty), from_ty);
    codegen.emit_store_acc_pair_impl(dest);
    return;
  }

  if (is_i128_type(from_ty) && is_float_type(to_ty) && !is_long_double_type(to_ty)) {
    codegen.emit_i128_to_float_call_impl(src, is_signed_int_type(from_ty), to_ty);
    codegen.emit_store_result_for_default_cast(dest);
    return;
  }

  if (is_i128_type(to_ty) && !is_i128_type(from_ty)) {
    codegen.emit_load_operand_for_return_default(src);
    if (type_size_bytes(from_ty) < 8) {
      codegen.emit_cast_instrs_impl(from_ty, is_signed_int_type(from_ty) ? IrType::I64 : IrType::U64);
    }
    if (is_signed_int_type(from_ty)) {
      codegen.emit_sign_extend_acc_high_impl();
    } else {
      codegen.emit_zero_acc_high_impl();
    }
    codegen.emit_store_acc_pair_impl(dest);
    return;
  }

  if (is_i128_type(from_ty) && !is_i128_type(to_ty)) {
    codegen.emit_load_acc_pair_impl(src);
    if (type_size_bytes(to_ty) < 8) {
      codegen.emit_cast_instrs_impl(IrType::I64, to_ty);
    }
    codegen.emit_store_result_for_default_cast(dest);
    return;
  }

  if (is_i128_type(from_ty) && is_i128_type(to_ty)) {
    codegen.emit_load_acc_pair_impl(src);
    codegen.emit_store_acc_pair_impl(dest);
    return;
  }

  codegen.emit_load_operand_for_return_default(src);
  codegen.emit_cast_instrs_impl(from_ty, to_ty);
  codegen.emit_store_result_for_default_cast(dest);
}

}  // namespace c4c::backend
