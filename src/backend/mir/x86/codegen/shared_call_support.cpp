#include "x86_codegen.hpp"
#include "../../regalloc.hpp"
#include "../../stack_layout/regalloc_helpers.hpp"

#include <limits>
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

void emit_zero_reg(X86Codegen& codegen, const char* reg) {
  codegen.state.emit(std::string("    xorl %") + reg_name_to_32_or_self(reg) + ", %" +
                     reg_name_to_32_or_self(reg));
}

}  // namespace

X86CodegenState::X86CodegenState() : out(this) {}

X86CodegenState::X86CodegenState(const X86CodegenState& other)
    : out(this),
      reg_cache(other.reg_cache),
      f128_direct_slots(other.f128_direct_slots),
      asm_lines(other.asm_lines),
      got_addr_symbols(other.got_addr_symbols),
      slots(other.slots),
      reg_assignment_indices(other.reg_assignment_indices),
      allocas(other.allocas),
      over_aligned_allocas(other.over_aligned_allocas),
      f128_load_sources(other.f128_load_sources),
      f128_constant_words(other.f128_constant_words),
      pic_mode(other.pic_mode) {}

X86CodegenState& X86CodegenState::operator=(const X86CodegenState& other) {
  if (this == &other) {
    return *this;
  }
  reg_cache = other.reg_cache;
  f128_direct_slots = other.f128_direct_slots;
  asm_lines = other.asm_lines;
  got_addr_symbols = other.got_addr_symbols;
  slots = other.slots;
  reg_assignment_indices = other.reg_assignment_indices;
  allocas = other.allocas;
  over_aligned_allocas = other.over_aligned_allocas;
  f128_load_sources = other.f128_load_sources;
  f128_constant_words = other.f128_constant_words;
  pic_mode = other.pic_mode;
  rebind_output();
  return *this;
}

X86CodegenState::X86CodegenState(X86CodegenState&& other) noexcept
    : out(this),
      reg_cache(std::move(other.reg_cache)),
      f128_direct_slots(std::move(other.f128_direct_slots)),
      asm_lines(std::move(other.asm_lines)),
      got_addr_symbols(std::move(other.got_addr_symbols)),
      slots(std::move(other.slots)),
      reg_assignment_indices(std::move(other.reg_assignment_indices)),
      allocas(std::move(other.allocas)),
      over_aligned_allocas(std::move(other.over_aligned_allocas)),
      f128_load_sources(std::move(other.f128_load_sources)),
      f128_constant_words(std::move(other.f128_constant_words)),
      pic_mode(other.pic_mode) {}

X86CodegenState& X86CodegenState::operator=(X86CodegenState&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  reg_cache = std::move(other.reg_cache);
  f128_direct_slots = std::move(other.f128_direct_slots);
  asm_lines = std::move(other.asm_lines);
  got_addr_symbols = std::move(other.got_addr_symbols);
  slots = std::move(other.slots);
  reg_assignment_indices = std::move(other.reg_assignment_indices);
  allocas = std::move(other.allocas);
  over_aligned_allocas = std::move(other.over_aligned_allocas);
  f128_load_sources = std::move(other.f128_load_sources);
  f128_constant_words = std::move(other.f128_constant_words);
  pic_mode = other.pic_mode;
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

void X86CodegenOutput::emit_instr_reg_rbp(const char* mnemonic,
                                          const char* reg,
                                          std::int64_t offset) {
  append_asm_line(this, std::string(mnemonic) + " " + format_reg(reg) + ", " +
                            std::to_string(offset) + "(%rbp)");
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

void X86CodegenState::mark_needs_got_for_addr(std::string name) {
  got_addr_symbols.insert(std::move(name));
}

bool X86CodegenState::needs_got_for_addr(std::string_view name) const {
  return got_addr_symbols.find(std::string(name)) != got_addr_symbols.end();
}

void collect_inline_asm_callee_saved_x86(const IrFunction& /*func*/,
                                         std::vector<c4c::backend::PhysReg>& /*asm_clobbered_regs*/) {}

std::vector<c4c::backend::PhysReg> filter_available_regs(
    const std::vector<c4c::backend::PhysReg>& callee_saved,
    const std::vector<c4c::backend::PhysReg>& asm_clobbered) {
  return c4c::backend::stack_layout::filter_available_regs(callee_saved, asm_clobbered);
}

std::pair<std::unordered_map<std::string, std::uint8_t>,
          std::optional<c4c::backend::LivenessResult>>
run_regalloc_and_merge_clobbers(
    const IrFunction& /*func*/,
    const std::vector<c4c::backend::PhysReg>& /*available_regs*/,
    const std::vector<c4c::backend::PhysReg>& /*caller_saved_regs*/,
    const std::vector<c4c::backend::PhysReg>& /*asm_clobbered_regs*/,
    std::unordered_map<std::string, std::uint8_t>& reg_assignments,
    std::vector<std::uint8_t>& used_callee_saved,
    bool /*allow_inline_asm_regalloc*/) {
  (void)used_callee_saved;
  return {reg_assignments, std::nullopt};
}

std::int64_t calculate_stack_space_common(
    X86CodegenState& /*state*/,
    const IrFunction& /*func*/,
    std::int64_t initial_space,
    const std::function<std::pair<std::int64_t, std::int64_t>(
        std::int64_t, std::int64_t, std::int64_t)>& /*alloc_fn*/,
    const std::unordered_map<std::string, std::uint8_t>& /*reg_assigned*/,
    const std::vector<c4c::backend::PhysReg>& /*callee_saved*/,
    const std::optional<c4c::backend::LivenessResult>& /*cached_liveness*/,
    bool /*track_param_allocas*/) {
  return initial_space;
}

std::optional<std::int64_t> X86Codegen::const_as_imm32(const Operand& op) const {
  if (!op.immediate.has_value()) {
    return std::nullopt;
  }
  if (*op.immediate < static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()) ||
      *op.immediate > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())) {
    return std::nullopt;
  }
  return *op.immediate;
}

void X86Codegen::operand_to_reg(const Operand& op, const char* reg) {
  if (const auto imm = this->const_as_imm32(op)) {
    if (*imm == 0) {
      emit_zero_reg(*this, reg);
    } else {
      this->state.out.emit_instr_imm_reg("    movq", *imm, reg);
    }
    return;
  }

  if (const auto assigned = this->state.assigned_reg_index(op.raw)) {
    const auto source = phys_reg_name(c4c::backend::PhysReg{*assigned});
    if (std::string_view(source) != reg) {
      this->state.emit(std::string("    movq %") + source + ", %" + reg);
    }
    return;
  }

  if (const auto addr = this->state.resolve_slot_addr(op.raw)) {
    switch (addr->kind) {
      case SlotAddr::Kind::Direct:
        this->state.out.emit_instr_rbp_reg("    movq", addr->slot.raw, reg);
        return;
      case SlotAddr::Kind::Indirect:
      case SlotAddr::Kind::OverAligned:
        this->emit_alloca_addr_to(reg, op.raw, addr->slot.raw);
        return;
    }
  }

  emit_zero_reg(*this, reg);
}

void X86Codegen::operand_to_rax(const Operand& op) {
  this->operand_to_reg(op, "rax");
  this->state.reg_cache.invalidate_acc();
}

void X86Codegen::operand_to_rcx(const Operand& op) {
  this->operand_to_reg(op, "rcx");
}

void X86Codegen::operand_to_rax_rdx(const Operand& op) {
  if (const auto imm = op.immediate) {
    if (*imm == 0) {
      this->state.emit("    xorl %eax, %eax");
      this->state.emit("    xorl %edx, %edx");
    } else {
      this->state.out.emit_instr_imm_reg("    movq", *imm, "rax");
      this->state.emit(*imm < 0 ? "    movq $-1, %rdx" : "    xorl %edx, %edx");
    }
  } else if (const auto slot = this->state.get_slot(op.raw)) {
    this->state.out.emit_instr_rbp_reg("    movq", slot->raw, "rax");
    this->state.out.emit_instr_rbp_reg("    movq", slot->raw + 8, "rdx");
  }
  this->state.reg_cache.invalidate_acc();
}

void X86Codegen::prep_i128_binop(const Operand& lhs, const Operand& rhs) {
  this->operand_to_rax_rdx(lhs);
  this->state.emit("    pushq %rdx");
  this->state.emit("    pushq %rax");
  this->operand_to_rax_rdx(rhs);
  this->state.emit("    movq %rax, %rcx");
  this->state.emit("    movq %rdx, %rsi");
  this->state.emit("    popq %rax");
  this->state.emit("    popq %rdx");
  this->state.reg_cache.invalidate_all();
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


}  // namespace c4c::backend::x86
