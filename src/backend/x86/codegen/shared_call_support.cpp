#include "x86_codegen.hpp"
#include "../../regalloc.hpp"

#include <string>

namespace c4c::backend::x86 {

namespace {

const char* reg_name_to_32_or_self(const char* reg) {
  const char* narrowed = reg_name_to_32(reg);
  return narrowed[0] == '\0' ? reg : narrowed;
}

std::string format_reg(const char* reg) { return std::string("%") + reg; }

void append_asm_line(X86CodegenOutput* out, std::string line) {
  if (out == nullptr || out->owner == nullptr) {
    return;
  }
  out->owner->asm_lines.push_back(std::move(line));
}

}  // namespace

X86CodegenState::X86CodegenState() : out(this) {}

X86CodegenState::X86CodegenState(const X86CodegenState& other)
    : out(this),
      reg_cache(other.reg_cache),
      f128_direct_slots(other.f128_direct_slots),
      asm_lines(other.asm_lines),
      slots(other.slots),
      reg_assignment_indices(other.reg_assignment_indices),
      allocas(other.allocas),
      over_aligned_allocas(other.over_aligned_allocas),
      f128_load_sources(other.f128_load_sources),
      f128_constant_words(other.f128_constant_words) {}

X86CodegenState& X86CodegenState::operator=(const X86CodegenState& other) {
  if (this == &other) {
    return *this;
  }
  reg_cache = other.reg_cache;
  f128_direct_slots = other.f128_direct_slots;
  asm_lines = other.asm_lines;
  slots = other.slots;
  reg_assignment_indices = other.reg_assignment_indices;
  allocas = other.allocas;
  over_aligned_allocas = other.over_aligned_allocas;
  f128_load_sources = other.f128_load_sources;
  f128_constant_words = other.f128_constant_words;
  rebind_output();
  return *this;
}

X86CodegenState::X86CodegenState(X86CodegenState&& other) noexcept
    : out(this),
      reg_cache(std::move(other.reg_cache)),
      f128_direct_slots(std::move(other.f128_direct_slots)),
      asm_lines(std::move(other.asm_lines)),
      slots(std::move(other.slots)),
      reg_assignment_indices(std::move(other.reg_assignment_indices)),
      allocas(std::move(other.allocas)),
      over_aligned_allocas(std::move(other.over_aligned_allocas)),
      f128_load_sources(std::move(other.f128_load_sources)),
      f128_constant_words(std::move(other.f128_constant_words)) {}

X86CodegenState& X86CodegenState::operator=(X86CodegenState&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  reg_cache = std::move(other.reg_cache);
  f128_direct_slots = std::move(other.f128_direct_slots);
  asm_lines = std::move(other.asm_lines);
  slots = std::move(other.slots);
  reg_assignment_indices = std::move(other.reg_assignment_indices);
  allocas = std::move(other.allocas);
  over_aligned_allocas = std::move(other.over_aligned_allocas);
  f128_load_sources = std::move(other.f128_load_sources);
  f128_constant_words = std::move(other.f128_constant_words);
  rebind_output();
  return *this;
}

void X86CodegenState::rebind_output() { out.bind(this); }

void X86CodegenOutput::emit_instr_imm_reg(const char* mnemonic,
                                          std::int64_t imm,
                                          const char* reg) {
  append_asm_line(this, std::string(mnemonic) + " $" + std::to_string(imm) + ", " +
                            format_reg(reg));
}

void X86CodegenOutput::emit_instr_rbp_reg(const char* mnemonic,
                                          std::int64_t offset,
                                          const char* reg) {
  append_asm_line(this, std::string(mnemonic) + " " + std::to_string(offset) + "(%rbp), " +
                            format_reg(reg));
}

void X86CodegenOutput::emit_instr_rbp(const char* mnemonic, std::int64_t offset) {
  append_asm_line(this, std::string(mnemonic) + " " + std::to_string(offset) + "(%rbp)");
}

void X86CodegenOutput::emit_instr_mem_reg(const char* mnemonic,
                                          std::int64_t offset,
                                          const char* base_reg,
                                          const char* dest_reg) {
  append_asm_line(this, std::string(mnemonic) + " " + std::to_string(offset) + "(" +
                            format_reg(base_reg) + "), " + format_reg(dest_reg));
}

void X86CodegenRegCache::invalidate_all() {
  acc_value_id.reset();
  acc_known_zero_extended = false;
  acc_valid = false;
}

void X86CodegenRegCache::invalidate_acc() {
  acc_value_id.reset();
  acc_known_zero_extended = false;
  acc_valid = false;
}

void X86CodegenRegCache::set_acc(std::uint32_t value_id, bool known_zero_extended) {
  acc_value_id = value_id;
  acc_known_zero_extended = known_zero_extended;
  acc_valid = true;
}

void X86CodegenState::emit(const std::string& asm_line) { asm_lines.push_back(asm_line); }

std::optional<StackSlot> X86CodegenState::get_slot(std::uint32_t value_id) const {
  const auto it = slots.find(value_id);
  if (it == slots.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<std::uint8_t> X86CodegenState::assigned_reg_index(std::uint32_t value_id) const {
  const auto it = reg_assignment_indices.find(value_id);
  if (it == reg_assignment_indices.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool X86CodegenState::is_alloca(std::uint32_t value_id) const {
  return allocas.find(value_id) != allocas.end();
}

std::optional<SlotAddr> X86CodegenState::resolve_slot_addr(std::uint32_t value_id) const {
  const auto slot = get_slot(value_id);
  if (!slot.has_value()) {
    return std::nullopt;
  }
  if (over_aligned_allocas.find(value_id) != over_aligned_allocas.end()) {
    return SlotAddr::OverAligned(*slot, value_id);
  }
  if (is_alloca(value_id)) {
    return SlotAddr::Indirect(*slot);
  }
  return SlotAddr::Direct(*slot);
}

void X86CodegenState::track_f128_load(std::uint32_t dest_id,
                                      std::uint32_t ptr_id,
                                      std::int64_t offset) {
  (void)offset;
  f128_load_sources[dest_id] = ptr_id;
}

std::optional<std::uint32_t> X86CodegenState::get_f128_source(std::uint32_t value_id) const {
  const auto it = f128_load_sources.find(value_id);
  if (it == f128_load_sources.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<std::size_t> X86CodegenState::alloca_over_align(std::uint32_t value_id) const {
  const auto it = over_aligned_allocas.find(value_id);
  if (it == over_aligned_allocas.end()) {
    return std::nullopt;
  }
  return it->second;
}

void X86CodegenState::set_f128_constant_words(std::uint32_t operand_id,
                                              std::uint64_t lo,
                                              std::uint64_t hi) {
  f128_constant_words[operand_id] = {lo, hi};
}

std::optional<std::array<std::uint64_t, 2>>
X86CodegenState::get_f128_constant_words(std::uint32_t operand_id) const {
  const auto it = f128_constant_words.find(operand_id);
  if (it == f128_constant_words.end()) {
    return std::nullopt;
  }
  return it->second;
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
