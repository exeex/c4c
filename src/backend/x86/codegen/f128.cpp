#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

std::optional<std::int64_t> X86Codegen::emit_f128_resolve_addr(const SlotAddr& addr,
                                                               std::uint32_t ptr_id,
                                                               std::int64_t offset) {
  this->state.emit("    <resolve-f128-addr>");
  return std::nullopt;
}

void X86Codegen::emit_f128_fldt(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset) {
  this->state.emit("    fldt <f128-source>");
}

void X86Codegen::emit_f128_fstpt(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset) {
  this->state.emit("    fstpt <f128-dest>");
}

void X86Codegen::emit_f128_store_raw_bytes(const SlotAddr& addr,
                                           std::uint32_t ptr_id,
                                           std::int64_t offset,
                                           std::uint64_t lo,
                                           std::uint64_t hi) {
  this->state.emit("    <store-f128-bytes>");
}

void X86Codegen::emit_f128_store_f64_via_x87(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset) {
  this->state.emit("    <store-f64-via-x87>");
}

void X86Codegen::emit_f128_load_finish(const Value& dest) {
  this->state.emit("    <finish-f128-load>");
}

void X86Codegen::emit_f128_to_int_from_memory(const SlotAddr& addr, IrType to_ty) {
  this->state.emit("    <f128-to-int-from-memory>");
}

void X86Codegen::emit_f128_st0_to_int(IrType to_ty) {
  this->state.emit("    <f128-st0-to-int>");
}

void X86Codegen::emit_cast_instrs_x86(IrType from_ty, IrType to_ty) {
  this->state.emit("    <x86-cast-helper>");
}

void X86Codegen::emit_int_to_f128_cast(IrType from_ty) { this->state.emit("    <int-to-f128-cast>"); }
void X86Codegen::emit_f128_to_int_cast(IrType to_ty) { this->state.emit("    <f128-to-int-cast>"); }
void X86Codegen::emit_f128_to_u64_cast() { this->state.emit("    <f128-to-u64-cast>"); }
void X86Codegen::emit_f128_to_f32_cast() { this->state.emit("    <f128-to-f32-cast>"); }
void X86Codegen::emit_fild_to_f64_via_stack() { this->state.emit("    <fild-to-f64>"); }
void X86Codegen::emit_fisttp_from_f64_via_stack() { this->state.emit("    <fisttp-from-f64>"); }
void X86Codegen::emit_extend_to_rax(IrType ty) { this->state.emit("    <extend-to-rax>"); }
void X86Codegen::emit_sign_extend_to_rax(IrType ty) { this->state.emit("    <sign-extend-to-rax>"); }
void X86Codegen::emit_zero_extend_to_rax(IrType ty) { this->state.emit("    <zero-extend-to-rax>"); }
void X86Codegen::emit_generic_cast(IrType from_ty, IrType to_ty) { this->state.emit("    <generic-cast>"); }
void X86Codegen::emit_float_to_unsigned(bool from_f64, bool to_u64, IrType to_ty) { this->state.emit("    <float-to-unsigned>"); }
void X86Codegen::emit_int_to_float_conv(bool to_f64) { this->state.emit("    <int-to-float-conv>"); }
void X86Codegen::emit_u64_to_float(bool to_f64) { this->state.emit("    <u64-to-float>"); }
void X86Codegen::emit_f128_load_to_x87(const Operand& operand) { this->state.emit("    <load-f128-to-x87>"); }

}  // namespace c4c::backend::x86
