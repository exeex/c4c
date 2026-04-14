// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/cast_ops.rs
// Structural mirror of the ref Rust source; a dedicated RISC-V C++ codegen
// header is not present yet, so this file keeps the translation local and
// method-shaped without inventing a fake public interface.
//
// //! RiscvCodegen: type conversion/casting operations.
// //
// // use crate::ir::reexports::{Operand, Value};
// // use crate::common::types::IrType;
// // use crate::backend::cast::{CastKind, classify_cast};
// // use super::emit::RiscvCodegen;
// //
// // impl RiscvCodegen {
// //     pub(super) fn emit_cast_instrs_impl(&mut self, from_ty: IrType, to_ty: IrType) {
// //         match classify_cast(from_ty, to_ty) {
// //             CastKind::Noop => {}
// //
// //             CastKind::UnsignedToSignedSameSize { to_ty } => {
// //                 if to_ty == IrType::I32 {
// //                     self.state.emit("    sext.w t0, t0");
// //                 }
// //             }
// //
// //             CastKind::FloatToSigned { from_f64 } => {
// //                 if from_f64 {
// //                     self.state.emit("    fmv.d.x ft0, t0");
// //                     self.state.emit("    fcvt.l.d t0, ft0, rtz");
// //                 } else {
// //                     self.state.emit("    fmv.w.x ft0, t0");
// //                     self.state.emit("    fcvt.l.s t0, ft0, rtz");
// //                 }
// //                 match to_ty {
// //                     IrType::I8 => {
// //                         self.state.emit("    slli t0, t0, 56");
// //                         self.state.emit("    srai t0, t0, 56");
// //                     }
// //                     IrType::I16 => {
// //                         self.state.emit("    slli t0, t0, 48");
// //                         self.state.emit("    srai t0, t0, 48");
// //                     }
// //                     IrType::I32 => self.state.emit("    sext.w t0, t0"),
// //                     _ => {}
// //                 }
// //             }
// //
// //             CastKind::FloatToUnsigned { from_f64, .. } => {
// //                 if from_f64 {
// //                     self.state.emit("    fmv.d.x ft0, t0");
// //                     self.state.emit("    fcvt.lu.d t0, ft0, rtz");
// //                 } else {
// //                     self.state.emit("    fmv.w.x ft0, t0");
// //                     self.state.emit("    fcvt.lu.s t0, ft0, rtz");
// //                 }
// //                 match to_ty {
// //                     IrType::U8 => self.state.emit("    andi t0, t0, 0xff"),
// //                     IrType::U16 => {
// //                         self.state.emit("    slli t0, t0, 48");
// //                         self.state.emit("    srli t0, t0, 48");
// //                     }
// //                     IrType::U32 => {
// //                         self.state.emit("    slli t0, t0, 32");
// //                         self.state.emit("    srli t0, t0, 32");
// //                     }
// //                     _ => {}
// //                 }
// //             }
// //
// //             CastKind::SignedToFloat { to_f64, from_ty } => {
// //                 match from_ty.size() {
// //                     1 => {
// //                         self.state.emit("    slli t0, t0, 56");
// //                         self.state.emit("    srai t0, t0, 56");
// //                     }
// //                     2 => {
// //                         self.state.emit("    slli t0, t0, 48");
// //                         self.state.emit("    srai t0, t0, 48");
// //                     }
// //                     4 => self.state.emit("    sext.w t0, t0"),
// //                     _ => {}
// //                 }
// //                 if to_f64 {
// //                     self.state.emit("    fcvt.d.l ft0, t0");
// //                     self.state.emit("    fmv.x.d t0, ft0");
// //                 } else {
// //                     self.state.emit("    fcvt.s.l ft0, t0");
// //                     self.state.emit("    fmv.x.w t0, ft0");
// //                 }
// //             }
// //
// //             CastKind::UnsignedToFloat { to_f64, from_ty } => {
// //                 let from_u64 = from_ty == IrType::U64;
// //                 if from_u64 {
// //                     if to_f64 {
// //                         self.state.emit("    fcvt.d.lu ft0, t0");
// //                         self.state.emit("    fmv.x.d t0, ft0");
// //                     } else {
// //                         self.state.emit("    fcvt.s.lu ft0, t0");
// //                         self.state.emit("    fmv.x.w t0, ft0");
// //                     }
// //                 } else if to_f64 {
// //                     self.state.emit("    fcvt.d.wu ft0, t0");
// //                     self.state.emit("    fmv.x.d t0, ft0");
// //                 } else {
// //                     self.state.emit("    fcvt.s.wu ft0, t0");
// //                     self.state.emit("    fmv.x.w t0, ft0");
// //                 }
// //             }
// //
// //             CastKind::FloatToFloat { widen } => {
// //                 if widen {
// //                     self.state.emit("    fmv.w.x ft0, t0");
// //                     self.state.emit("    fcvt.d.s ft0, ft0");
// //                     self.state.emit("    fmv.x.d t0, ft0");
// //                 } else {
// //                     self.state.emit("    fmv.d.x ft0, t0");
// //                     self.state.emit("    fcvt.s.d ft0, ft0");
// //                     self.state.emit("    fmv.x.w t0, ft0");
// //                 }
// //             }
// //
// //             CastKind::SignedToUnsignedSameSize { to_ty } => {
// //                 match to_ty {
// //                     IrType::U8 => self.state.emit("    andi t0, t0, 0xff"),
// //                     IrType::U16 => {
// //                         self.state.emit("    slli t0, t0, 48");
// //                         self.state.emit("    srli t0, t0, 48");
// //                     }
// //                     IrType::U32 => {
// //                         self.state.emit("    slli t0, t0, 32");
// //                         self.state.emit("    srli t0, t0, 32");
// //                     }
// //                     _ => {}
// //                 }
// //             }
// //
// //             CastKind::IntWiden { from_ty, .. } => {
// //                 if from_ty.is_unsigned() {
// //                     match from_ty {
// //                         IrType::U8 => self.state.emit("    andi t0, t0, 0xff"),
// //                         IrType::U16 => {
// //                             self.state.emit("    slli t0, t0, 48");
// //                             self.state.emit("    srli t0, t0, 48");
// //                         }
// //                         IrType::U32 => {
// //                             self.state.emit("    slli t0, t0, 32");
// //                             self.state.emit("    srli t0, t0, 32");
// //                         }
// //                         _ => {}
// //                     }
// //                 } else {
// //                     match from_ty {
// //                         IrType::I8 => {
// //                             self.state.emit("    slli t0, t0, 56");
// //                             self.state.emit("    srai t0, t0, 56");
// //                         }
// //                         IrType::I16 => {
// //                             self.state.emit("    slli t0, t0, 48");
// //                             self.state.emit("    srai t0, t0, 48");
// //                         }
// //                         IrType::I32 => self.state.emit("    sext.w t0, t0"),
// //                         _ => {}
// //                     }
// //                 }
// //             }
// //
// //             CastKind::IntNarrow { to_ty } => {
// //                 match to_ty {
// //                     IrType::I8 => {
// //                         self.state.emit("    slli t0, t0, 56");
// //                         self.state.emit("    srai t0, t0, 56");
// //                     }
// //                     IrType::U8 => self.state.emit("    andi t0, t0, 0xff"),
// //                     IrType::I16 => {
// //                         self.state.emit("    slli t0, t0, 48");
// //                         self.state.emit("    srai t0, t0, 48");
// //                     }
// //                     IrType::U16 => {
// //                         self.state.emit("    slli t0, t0, 48");
// //                         self.state.emit("    srli t0, t0, 48");
// //                     }
// //                     IrType::I32 => self.state.emit("    sext.w t0, t0"),
// //                     IrType::U32 => {
// //                         self.state.emit("    slli t0, t0, 32");
// //                         self.state.emit("    srli t0, t0, 32");
// //                     }
// //                     _ => {}
// //                 }
// //             }
// //
// //             CastKind::SignedToF128 { .. }
// //             | CastKind::UnsignedToF128 { .. }
// //             | CastKind::F128ToSigned { .. }
// //             | CastKind::F128ToUnsigned { .. }
// //             | CastKind::FloatToF128 { .. }
// //             | CastKind::F128ToFloat { .. } => {
// //                 unreachable!("F128 casts should be handled by emit_cast override");
// //             }
// //         }
// //     }
// //
// //     pub(super) fn emit_cast_impl(&mut self, dest: &Value, src: &Operand, from_ty: IrType, to_ty: IrType) {
// //         if crate::backend::f128_softfloat::f128_emit_cast(self, dest, src, from_ty, to_ty) {
// //             return;
// //         }
// //         crate::backend::traits::emit_cast_default(self, dest, src, from_ty, to_ty);
// //     }
// // }

#include "riscv_codegen.hpp"
#include "../../f128_softfloat.hpp"

#include <cstddef>

namespace c4c::backend::riscv::codegen {

namespace {

constexpr bool is_float_ty(IrType ty) {
  return ty == IrType::F32 || ty == IrType::F64;
}

constexpr bool is_unsigned_ty(IrType ty) {
  return ty == IrType::U16 || ty == IrType::U32 || ty == IrType::U64 || ty == IrType::Ptr;
}

constexpr bool is_signed_ty(IrType ty) {
  return ty == IrType::I8 || ty == IrType::I16 || ty == IrType::I32 || ty == IrType::I64;
}

constexpr std::size_t type_size_bytes(IrType ty) {
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
}

constexpr IrType normalize_ptr_ty(IrType ty) {
  return ty == IrType::Ptr ? IrType::U64 : ty;
}

}  // namespace

void RiscvCodegen::emit_cast_instrs_impl(IrType from_ty, IrType to_ty) {
  const auto effective_from = normalize_ptr_ty(from_ty);
  const auto effective_to = normalize_ptr_ty(to_ty);

  if (effective_from == effective_to) {
    return;
  }

  if (is_float_ty(effective_from) && !is_float_ty(effective_to)) {
    if (effective_from == IrType::F64) {
      state.emit("    fmv.d.x ft0, t0");
      state.emit("    fcvt.l" + std::string(is_unsigned_ty(effective_to) ? "u" : "") + ".d t0, ft0, rtz");
    } else {
      state.emit("    fmv.w.x ft0, t0");
      state.emit("    fcvt.l" + std::string(is_unsigned_ty(effective_to) ? "u" : "") + ".s t0, ft0, rtz");
    }

    switch (effective_to) {
      case IrType::I8:
        state.emit("    slli t0, t0, 56");
        state.emit("    srai t0, t0, 56");
        return;
      case IrType::I16:
        state.emit("    slli t0, t0, 48");
        state.emit("    srai t0, t0, 48");
        return;
      case IrType::U16:
        state.emit("    slli t0, t0, 48");
        state.emit("    srli t0, t0, 48");
        return;
      case IrType::I32:
        state.emit("    sext.w t0, t0");
        return;
      case IrType::U32:
        state.emit("    slli t0, t0, 32");
        state.emit("    srli t0, t0, 32");
        return;
      default: return;
    }
  }

  if (!is_float_ty(effective_from) && is_float_ty(effective_to)) {
    if (is_signed_ty(effective_from)) {
      switch (type_size_bytes(effective_from)) {
        case 1:
          state.emit("    slli t0, t0, 56");
          state.emit("    srai t0, t0, 56");
          break;
        case 2:
          state.emit("    slli t0, t0, 48");
          state.emit("    srai t0, t0, 48");
          break;
        case 4:
          state.emit("    sext.w t0, t0");
          break;
        default:
          break;
      }
    }

    const auto to_f64 = effective_to == IrType::F64;
    if (is_unsigned_ty(effective_from)) {
      const auto from_u64 = effective_from == IrType::U64 || effective_from == IrType::Ptr;
      if (from_u64) {
        state.emit(std::string("    fcvt.") + (to_f64 ? "d" : "s") + ".lu ft0, t0");
      } else {
        state.emit(std::string("    fcvt.") + (to_f64 ? "d" : "s") + ".wu ft0, t0");
      }
    } else {
      state.emit(std::string("    fcvt.") + (to_f64 ? "d" : "s") + ".l ft0, t0");
    }

    state.emit(std::string("    fmv.x.") + (to_f64 ? "d" : "w") + " t0, ft0");
    return;
  }

  if (is_float_ty(effective_from) && is_float_ty(effective_to)) {
    if (effective_from == IrType::F32 && effective_to == IrType::F64) {
      state.emit("    fmv.w.x ft0, t0");
      state.emit("    fcvt.d.s ft0, ft0");
      state.emit("    fmv.x.d t0, ft0");
      return;
    }

    state.emit("    fmv.d.x ft0, t0");
    state.emit("    fcvt.s.d ft0, ft0");
    state.emit("    fmv.x.w t0, ft0");
    return;
  }

  if (type_size_bytes(effective_from) == type_size_bytes(effective_to)) {
    if (is_unsigned_ty(effective_from) && effective_to == IrType::I32) {
      state.emit("    sext.w t0, t0");
      return;
    }

    if (effective_to == IrType::U16) {
      state.emit("    slli t0, t0, 48");
      state.emit("    srli t0, t0, 48");
      return;
    }

    if (effective_to == IrType::U32) {
      state.emit("    slli t0, t0, 32");
      state.emit("    srli t0, t0, 32");
    }
    return;
  }

  if (type_size_bytes(effective_to) > type_size_bytes(effective_from)) {
    if (is_unsigned_ty(effective_from)) {
      switch (effective_from) {
        case IrType::U16:
          state.emit("    slli t0, t0, 48");
          state.emit("    srli t0, t0, 48");
          return;
        case IrType::U32:
          state.emit("    slli t0, t0, 32");
          state.emit("    srli t0, t0, 32");
          return;
        default:
          return;
      }
    }

    switch (effective_from) {
      case IrType::I8:
        state.emit("    slli t0, t0, 56");
        state.emit("    srai t0, t0, 56");
        return;
      case IrType::I16:
        state.emit("    slli t0, t0, 48");
        state.emit("    srai t0, t0, 48");
        return;
      case IrType::I32:
        state.emit("    sext.w t0, t0");
        return;
      default:
        return;
    }
  }

  switch (effective_to) {
    case IrType::I8:
      state.emit("    slli t0, t0, 56");
      state.emit("    srai t0, t0, 56");
      return;
    case IrType::I16:
      state.emit("    slli t0, t0, 48");
      state.emit("    srai t0, t0, 48");
      return;
    case IrType::U16:
      state.emit("    slli t0, t0, 48");
      state.emit("    srli t0, t0, 48");
      return;
    case IrType::I32:
      state.emit("    sext.w t0, t0");
      return;
    case IrType::U32:
      state.emit("    slli t0, t0, 32");
      state.emit("    srli t0, t0, 32");
      return;
    default:
      return;
  }
}

void RiscvCodegen::emit_cast_impl(const Value& dest,
                                  const Operand& src,
                                  IrType from_ty,
                                  IrType to_ty) {
  if (c4c::backend::emit_f128_cast(*this, dest, src, from_ty, to_ty)) {
    return;
  }

  c4c::backend::emit_cast_default(*this, dest, src, from_ty, to_ty);
}

}  // namespace c4c::backend::riscv::codegen
