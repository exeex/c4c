#include "x86_codegen.hpp"

#include <string>

namespace c4c::backend::x86 {

namespace {

// Transitional shared call-owner support: this slice only turns the missing
// helper/state declarations into real linkable definitions so calls.cpp can
// enter the active build while emit.cpp still owns the runtime x86 path.
const char* reg_name_to_32_or_self(const char* reg) {
  const char* narrowed = reg_name_to_32(reg);
  return narrowed[0] == '\0' ? reg : narrowed;
}

}  // namespace

void X86CodegenOutput::emit_instr_imm_reg(const char* mnemonic,
                                          std::int64_t imm,
                                          const char* reg) {
  (void)mnemonic;
  (void)imm;
  (void)reg;
}

void X86CodegenOutput::emit_instr_rbp_reg(const char* mnemonic,
                                          std::int64_t offset,
                                          const char* reg) {
  (void)mnemonic;
  (void)offset;
  (void)reg;
}

void X86CodegenOutput::emit_instr_rbp(const char* mnemonic, std::int64_t offset) {
  (void)mnemonic;
  (void)offset;
}

void X86CodegenOutput::emit_instr_mem_reg(const char* mnemonic,
                                          std::int64_t offset,
                                          const char* base_reg,
                                          const char* dest_reg) {
  (void)mnemonic;
  (void)offset;
  (void)base_reg;
  (void)dest_reg;
}

void X86CodegenRegCache::invalidate_all() {}

void X86CodegenRegCache::invalidate_acc() {}

void X86CodegenRegCache::set_acc(std::uint32_t value_id, bool known_zero_extended) {
  (void)value_id;
  (void)known_zero_extended;
}

void X86CodegenState::emit(const std::string& asm_line) { (void)asm_line; }

std::optional<StackSlot> X86CodegenState::get_slot(std::uint32_t value_id) const {
  (void)value_id;
  return std::nullopt;
}

bool X86CodegenState::is_alloca(std::uint32_t value_id) const {
  (void)value_id;
  return false;
}

std::optional<SlotAddr> X86CodegenState::resolve_slot_addr(std::uint32_t value_id) const {
  (void)value_id;
  return std::nullopt;
}

void X86CodegenState::track_f128_load(std::uint32_t dest_id,
                                      std::uint32_t ptr_id,
                                      std::int64_t offset) {
  (void)dest_id;
  (void)ptr_id;
  (void)offset;
}

std::optional<std::uint32_t> X86CodegenState::get_f128_source(std::uint32_t value_id) const {
  (void)value_id;
  return std::nullopt;
}

std::optional<std::size_t> X86CodegenState::alloca_over_align(std::uint32_t value_id) const {
  (void)value_id;
  return std::nullopt;
}

void X86Codegen::operand_to_rax(const Operand& op) {
  if (const auto slot = this->state.get_slot(op.raw)) {
    this->state.out.emit_instr_rbp_reg("    movq", slot->raw, "rax");
  }
  this->state.reg_cache.invalidate_acc();
}

void X86Codegen::operand_to_rcx(const Operand& op) {
  if (const auto slot = this->state.get_slot(op.raw)) {
    this->state.out.emit_instr_rbp_reg("    movq", slot->raw, "rcx");
  }
}

void X86Codegen::operand_to_rax_rdx(const Operand& op) {
  if (const auto slot = this->state.get_slot(op.raw)) {
    this->state.out.emit_instr_rbp_reg("    movq", slot->raw, "rax");
    this->state.out.emit_instr_rbp_reg("    movq", slot->raw + 8, "rdx");
  }
  this->state.reg_cache.invalidate_acc();
}

void X86Codegen::store_rax_to(const Value& dest) {
  if (const auto slot = this->state.get_slot(dest.raw)) {
    this->state.emit("    movq %rax, " + std::to_string(slot->raw) + "(%rbp)");
  }
  this->state.reg_cache.set_acc(dest.raw, false);
}

void X86Codegen::store_rax_rdx_to(const Value& dest) {
  if (const auto slot = this->state.get_slot(dest.raw)) {
    this->state.emit("    movq %rax, " + std::to_string(slot->raw) + "(%rbp)");
    this->state.emit("    movq %rdx, " + std::to_string(slot->raw + 8) + "(%rbp)");
  }
  this->state.reg_cache.invalidate_all();
}

const char* X86Codegen::reg_for_type(const char* reg, IrType ty) const {
  switch (ty) {
    case IrType::I32:
    case IrType::F32: return reg_name_to_32_or_self(reg);
    default: return reg;
  }
}

const char* X86Codegen::mov_load_for_type(IrType ty) const {
  switch (ty) {
    case IrType::I32:
    case IrType::F32: return "movl";
    default: return "movq";
  }
}

const char* X86Codegen::mov_store_for_type(IrType ty) const {
  switch (ty) {
    case IrType::I32:
    case IrType::F32: return "movl";
    default: return "movq";
  }
}

const char* X86Codegen::type_suffix(IrType ty) const {
  switch (ty) {
    case IrType::I32:
    case IrType::F32: return "l";
    default: return "q";
  }
}

}  // namespace c4c::backend::x86
