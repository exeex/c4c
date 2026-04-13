#include "f128_softfloat.hpp"

#include "riscv/codegen/riscv_codegen.hpp"

#include <cstdint>
#include <string>

namespace c4c::backend {

namespace {

using FloatOp = riscv::codegen::FloatOp;
using IrCmpOp = riscv::codegen::IrCmpOp;
using Operand = riscv::codegen::Operand;
using RiscvCodegen = riscv::codegen::RiscvCodegen;
using Value = riscv::codegen::Value;

void emit_riscv_store_result_and_truncate(RiscvCodegen& codegen, const Value& dest) {
  if (const auto slot = codegen.state.get_slot(dest.raw)) {
    codegen.state.emit("    sd a0, " + std::to_string(slot->raw) + "(s0)");
    codegen.state.emit("    sd a1, " + std::to_string(slot->raw + 8) + "(s0)");
  }

  codegen.state.emit("    call __trunctfdf2");
  codegen.state.emit("    fmv.x.d t0, fa0");

  if (const auto reg = codegen.state.assigned_reg(dest.raw)) {
    codegen.state.emit("    mv " + std::string(riscv::codegen::callee_saved_name(*reg)) + ", t0");
  }
}

const char* riscv_f128_binop_libcall(FloatOp op) {
  switch (op) {
    case FloatOp::Add: return "__addtf3";
    case FloatOp::Sub: return "__subtf3";
    case FloatOp::Mul: return "__multf3";
    case FloatOp::Div: return "__divtf3";
  }
  return "__addtf3";
}

std::pair<const char*, F128CmpKind> riscv_f128_cmp_libcall(IrCmpOp op) {
  switch (op) {
    case IrCmpOp::Eq: return {"__eqtf2", F128CmpKind::Eq};
    case IrCmpOp::Ne: return {"__eqtf2", F128CmpKind::Ne};
    case IrCmpOp::Slt:
    case IrCmpOp::Ult: return {"__lttf2", F128CmpKind::Lt};
    case IrCmpOp::Sle:
    case IrCmpOp::Ule: return {"__letf2", F128CmpKind::Le};
    case IrCmpOp::Sgt:
    case IrCmpOp::Ugt: return {"__gttf2", F128CmpKind::Gt};
    case IrCmpOp::Sge:
    case IrCmpOp::Uge: return {"__getf2", F128CmpKind::Ge};
  }
  return {"__eqtf2", F128CmpKind::Eq};
}

}  // namespace

void emit_riscv_f128_store_with_offset(RiscvCodegen& codegen,
                                       const Operand& value,
                                       const Value& base,
                                       std::int64_t offset) {
  emit_f128_operand_to_arg1(codegen, value);

  const auto addr = codegen.state.resolve_slot_addr(base.raw);
  if (!addr.has_value()) {
    return;
  }

  switch (addr->kind) {
    case riscv::codegen::SlotAddr::Kind::Direct:
      codegen.f128_store_arg1_to_frame_offset(addr->slot.raw + offset);
      return;
    case riscv::codegen::SlotAddr::Kind::OverAligned:
      codegen.emit_alloca_aligned_addr_impl(addr->slot, addr->value_id);
      if (offset != 0) {
        codegen.emit_add_offset_to_addr_reg_impl(offset);
      }
      codegen.f128_store_arg1_to_addr();
      return;
    case riscv::codegen::SlotAddr::Kind::Indirect:
      codegen.emit_load_ptr_from_slot_impl(addr->slot, static_cast<std::uint32_t>(base.raw));
      if (offset != 0) {
        codegen.emit_add_offset_to_addr_reg_impl(offset);
      }
      codegen.f128_store_arg1_to_addr();
      return;
  }
}

void emit_riscv_f128_store(RiscvCodegen& codegen, const Operand& value, const Value& ptr) {
  emit_riscv_f128_store_with_offset(codegen, value, ptr, 0);
}

void emit_riscv_f128_load_with_offset(RiscvCodegen& codegen,
                                      const Value& dest,
                                      const Value& base,
                                      std::int64_t offset) {
  const auto addr = codegen.state.resolve_slot_addr(base.raw);
  if (!addr.has_value()) {
    return;
  }

  switch (addr->kind) {
    case riscv::codegen::SlotAddr::Kind::Direct:
      codegen.f128_load_from_frame_offset_to_arg1(addr->slot.raw + offset);
      break;
    case riscv::codegen::SlotAddr::Kind::OverAligned:
      codegen.emit_alloca_aligned_addr_impl(addr->slot, addr->value_id);
      if (offset != 0) {
        codegen.emit_add_offset_to_addr_reg_impl(offset);
      }
      codegen.f128_load_from_addr_reg_to_arg1();
      break;
    case riscv::codegen::SlotAddr::Kind::Indirect:
      codegen.emit_load_ptr_from_slot_impl(addr->slot, static_cast<std::uint32_t>(base.raw));
      if (offset != 0) {
        codegen.emit_add_offset_to_addr_reg_impl(offset);
      }
      codegen.f128_load_from_addr_reg_to_arg1();
      break;
  }

  emit_riscv_store_result_and_truncate(codegen, dest);
}

void emit_riscv_f128_load(RiscvCodegen& codegen, const Value& dest, const Value& ptr) {
  emit_riscv_f128_load_with_offset(codegen, dest, ptr, 0);
}

void emit_f128_operand_to_arg1(F128SoftFloatHooks& codegen, const Operand& operand) {
  if (operand.kind == Operand::Kind::Value) {
    std::int64_t offset = 0;
    if (codegen.f128_try_get_frame_offset(operand.raw, offset)) {
      codegen.f128_load_from_frame_offset_to_arg1(offset);
      return;
    }
  }

  codegen.f128_load_operand_and_extend(operand);
}

bool emit_f128_cast(F128SoftFloatHooks& codegen,
                    const Value& dest,
                    const Operand& src,
                    riscv::codegen::IrType from_ty,
                    riscv::codegen::IrType to_ty) {
  using IrType = riscv::codegen::IrType;

  const auto is_float_ty = [](IrType ty) {
    return ty == IrType::F32 || ty == IrType::F64 || ty == IrType::F128;
  };
  const auto is_i128_ty = [](IrType ty) { return ty == IrType::I128; };
  const auto is_unsigned_ty = [](IrType ty) {
    return ty == IrType::U16 || ty == IrType::U32 || ty == IrType::U64 || ty == IrType::Ptr;
  };
  const auto type_size_bytes = [](IrType ty) -> std::size_t {
    switch (ty) {
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
  };

  if (to_ty == IrType::F128 && !is_float_ty(from_ty) && !is_i128_ty(from_ty)) {
    codegen.f128_load_operand_to_acc(src);
    if (is_unsigned_ty(from_ty)) {
      codegen.f128_zero_extend_acc(type_size_bytes(from_ty));
      codegen.f128_move_acc_to_arg0();
      codegen.f128_call("__floatunditf");
    } else {
      codegen.f128_sign_extend_acc(type_size_bytes(from_ty));
      codegen.f128_move_acc_to_arg0();
      codegen.f128_call("__floatditf");
    }
    codegen.f128_store_result_and_truncate(dest);
    return true;
  }

  if (from_ty == IrType::F128 && !is_float_ty(to_ty) && !is_i128_ty(to_ty)) {
    emit_f128_operand_to_arg1(codegen, src);
    codegen.f128_call(is_unsigned_ty(to_ty) ? "__fixunstfdi" : "__fixtfdi");
    codegen.f128_move_arg0_to_acc();
    if (type_size_bytes(to_ty) < 8) {
      codegen.f128_narrow_acc(to_ty);
    }
    codegen.f128_store_acc_to_dest(dest);
    return true;
  }

  if (to_ty == IrType::F128 && is_float_ty(from_ty) && from_ty != IrType::F128) {
    codegen.f128_load_operand_to_acc(src);
    codegen.f128_extend_float_to_f128(from_ty);
    codegen.f128_store_result_and_truncate(dest);
    return true;
  }

  if (from_ty == IrType::F128 && is_float_ty(to_ty) && to_ty != IrType::F128) {
    emit_f128_operand_to_arg1(codegen, src);
    codegen.f128_truncate_to_float_acc(to_ty);
    codegen.f128_store_acc_to_dest(dest);
    return true;
  }

  return false;
}

void emit_f128_neg(F128SoftFloatHooks& codegen, const Value& dest, const Operand& src) {
  emit_f128_operand_to_arg1(codegen, src);
  codegen.f128_flip_sign_bit();

  std::int64_t offset = 0;
  if (codegen.f128_try_get_frame_offset(dest.raw, offset)) {
    codegen.f128_store_arg1_to_frame_offset(offset);
    codegen.f128_track_self(dest.raw);
  }

  codegen.f128_truncate_result_to_acc();
  codegen.f128_set_acc_cache(dest.raw);
}

void emit_riscv_f128_binop(RiscvCodegen& codegen,
                           const Value& dest,
                           FloatOp op,
                           const Operand& lhs,
                           const Operand& rhs) {
  codegen.state.emit("    addi sp, sp, -16");
  emit_f128_operand_to_arg1(codegen, lhs);
  codegen.state.emit("    sd a0, 0(sp)");
  codegen.state.emit("    sd a1, 8(sp)");
  emit_f128_operand_to_arg1(codegen, rhs);
  codegen.state.emit("    mv a2, a0");
  codegen.state.emit("    mv a3, a1");
  codegen.state.emit("    ld a0, 0(sp)");
  codegen.state.emit("    ld a1, 8(sp)");
  codegen.state.emit("    addi sp, sp, 16");
  codegen.state.emit(std::string("    call ") + riscv_f128_binop_libcall(op));
  emit_riscv_store_result_and_truncate(codegen, dest);
}

void emit_riscv_f128_cmp(RiscvCodegen& codegen,
                         const Value& dest,
                         IrCmpOp op,
                         const Operand& lhs,
                         const Operand& rhs) {
  codegen.f128_alloc_temp_16();
  emit_f128_operand_to_arg1(codegen, lhs);
  codegen.f128_save_arg1_to_sp();
  emit_f128_operand_to_arg1(codegen, rhs);
  codegen.f128_move_arg1_to_arg2();
  codegen.f128_reload_arg1_from_sp();
  codegen.f128_free_temp_16();

  const auto [libcall, kind] = riscv_f128_cmp_libcall(op);
  codegen.f128_call(libcall);
  codegen.f128_cmp_result_to_bool(kind);
  codegen.f128_store_bool_result(dest);
}

}  // namespace c4c::backend
