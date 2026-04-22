#include "x86_codegen_output.hpp"

#include "../x86_codegen.hpp"

#include <cctype>
#include <limits>

namespace c4c::backend::x86 {

namespace {

constexpr const char* kX86ArgRegs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
constexpr const char* kX86FloatArgRegs[] = {
    "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"};

std::string format_reg(const char* reg) { return std::string("%") + reg; }

const char* reg_name_to_32_or_self(const char* reg) {
  const char* narrowed = reg_name_to_32(reg);
  return narrowed[0] == '\0' ? reg : narrowed;
}

std::string reg_name_to_64_or_self(std::string_view reg) {
  if (reg == "rax" || reg == "eax" || reg == "ax" || reg == "al") return "rax";
  if (reg == "rbx" || reg == "ebx" || reg == "bx" || reg == "bl") return "rbx";
  if (reg == "rcx" || reg == "ecx" || reg == "cx" || reg == "cl") return "rcx";
  if (reg == "rdx" || reg == "edx" || reg == "dx" || reg == "dl") return "rdx";
  if (reg == "rsi" || reg == "esi" || reg == "si" || reg == "sil") return "rsi";
  if (reg == "rdi" || reg == "edi" || reg == "di" || reg == "dil") return "rdi";
  if (reg == "rsp" || reg == "esp" || reg == "sp" || reg == "spl") return "rsp";
  if (reg == "rbp" || reg == "ebp" || reg == "bp" || reg == "bpl") return "rbp";
  if (reg == "r8" || reg == "r8d" || reg == "r8w" || reg == "r8b") return "r8";
  if (reg == "r9" || reg == "r9d" || reg == "r9w" || reg == "r9b") return "r9";
  if (reg == "r10" || reg == "r10d" || reg == "r10w" || reg == "r10b") return "r10";
  if (reg == "r11" || reg == "r11d" || reg == "r11w" || reg == "r11b") return "r11";
  if (reg == "r12" || reg == "r12d" || reg == "r12w" || reg == "r12b") return "r12";
  if (reg == "r13" || reg == "r13d" || reg == "r13w" || reg == "r13b") return "r13";
  if (reg == "r14" || reg == "r14d" || reg == "r14w" || reg == "r14b") return "r14";
  if (reg == "r15" || reg == "r15d" || reg == "r15w" || reg == "r15b") return "r15";
  return std::string(reg);
}

std::string reg_name_to_16_or_self(const char* reg) {
  const auto wide = reg_name_to_64_or_self(reg);
  if (wide == "rax") return "ax";
  if (wide == "rbx") return "bx";
  if (wide == "rcx") return "cx";
  if (wide == "rdx") return "dx";
  if (wide == "rsi") return "si";
  if (wide == "rdi") return "di";
  if (wide == "rsp") return "sp";
  if (wide == "rbp") return "bp";
  if (wide == "r8") return "r8w";
  if (wide == "r9") return "r9w";
  if (wide == "r10") return "r10w";
  if (wide == "r11") return "r11w";
  if (wide == "r12") return "r12w";
  if (wide == "r13") return "r13w";
  if (wide == "r14") return "r14w";
  if (wide == "r15") return "r15w";
  return std::string(reg);
}

std::string reg_name_to_8_or_self(const char* reg) {
  const auto wide = reg_name_to_64_or_self(reg);
  if (wide == "rax") return "al";
  if (wide == "rbx") return "bl";
  if (wide == "rcx") return "cl";
  if (wide == "rdx") return "dl";
  if (wide == "rsi") return "sil";
  if (wide == "rdi") return "dil";
  if (wide == "rsp") return "spl";
  if (wide == "rbp") return "bpl";
  if (wide == "r8") return "r8b";
  if (wide == "r9") return "r9b";
  if (wide == "r10") return "r10b";
  if (wide == "r11") return "r11b";
  if (wide == "r12") return "r12b";
  if (wide == "r13") return "r13b";
  if (wide == "r14") return "r14b";
  if (wide == "r15") return "r15b";
  return std::string(reg);
}

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

const char* load_dest_reg(IrType ty) {
  switch (ty) {
    case IrType::U32:
    case IrType::F32:
      return "%eax";
    default:
      return "%rax";
  }
}

std::string atomic_loop_label() {
  static std::uint64_t next_label_id = 0;
  return ".Latomic_loop_" + std::to_string(next_label_id++);
}

}  // namespace

const char* reg_name_to_32(std::string_view name) {
  if (name == "rax") return "eax";
  if (name == "rbx") return "ebx";
  if (name == "rcx") return "ecx";
  if (name == "rdx") return "edx";
  if (name == "rsi") return "esi";
  if (name == "rdi") return "edi";
  if (name == "rsp") return "esp";
  if (name == "rbp") return "ebp";
  if (name == "r8") return "r8d";
  if (name == "r9") return "r9d";
  if (name == "r10") return "r10d";
  if (name == "r11") return "r11d";
  if (name == "r12") return "r12d";
  if (name == "r13") return "r13d";
  if (name == "r14") return "r14d";
  if (name == "r15") return "r15d";
  return "";
}

const char* phys_reg_name(c4c::backend::PhysReg reg) {
  switch (reg.index) {
    case 1: return "rbx";
    case 2: return "r12";
    case 3: return "r13";
    case 4: return "r14";
    case 5: return "r15";
    case 10: return "r11";
    case 11: return "r10";
    case 12: return "r8";
    case 13: return "r9";
    case 14: return "rdi";
    case 15: return "rsi";
    default: return "";
  }
}

const char* x86_arg_reg_name(std::size_t reg_index) {
  return reg_index < std::size(kX86ArgRegs) ? kX86ArgRegs[reg_index] : "";
}

const char* x86_float_arg_reg_name(std::size_t reg_index) {
  return reg_index < std::size(kX86FloatArgRegs) ? kX86FloatArgRegs[reg_index] : "";
}

bool x86_allow_struct_split_reg_stack() { return false; }

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
      param_classes(other.param_classes),
      param_pre_stored(other.param_pre_stored),
      pic_mode(other.pic_mode),
      cf_protection_branch(other.cf_protection_branch),
      function_return_thunk(other.function_return_thunk) {}

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
  param_classes = other.param_classes;
  param_pre_stored = other.param_pre_stored;
  pic_mode = other.pic_mode;
  cf_protection_branch = other.cf_protection_branch;
  function_return_thunk = other.function_return_thunk;
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
      param_classes(std::move(other.param_classes)),
      param_pre_stored(std::move(other.param_pre_stored)),
      pic_mode(other.pic_mode),
      cf_protection_branch(other.cf_protection_branch),
      function_return_thunk(other.function_return_thunk) {}

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
  param_classes = std::move(other.param_classes);
  param_pre_stored = std::move(other.param_pre_stored);
  pic_mode = other.pic_mode;
  cf_protection_branch = other.cf_protection_branch;
  function_return_thunk = other.function_return_thunk;
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

void X86Codegen::emit_alloca_addr_to(const char* reg, std::uint32_t val_id, std::int64_t offset) {
  if (const auto align = this->state.alloca_over_align(val_id)) {
    this->state.out.emit_instr_rbp_reg("    leaq", offset, reg);
    this->state.out.emit_instr_imm_reg("    addq", static_cast<std::int64_t>(*align - 1), reg);
    this->state.out.emit_instr_imm_reg("    andq", -static_cast<std::int64_t>(*align), reg);
  } else {
    this->state.out.emit_instr_rbp_reg("    leaq", offset, reg);
  }
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
    case IrType::I8: {
      static thread_local std::string narrowed;
      narrowed = reg_name_to_8_or_self(reg);
      return narrowed.c_str();
    }
    case IrType::I16:
    case IrType::U16: {
      static thread_local std::string narrowed;
      narrowed = reg_name_to_16_or_self(reg);
      return narrowed.c_str();
    }
    case IrType::I32:
    case IrType::U32:
    case IrType::F32: return reg_name_to_32_or_self(reg);
    default: return reg;
  }
}

const char* X86Codegen::mov_load_for_type(IrType ty) const {
  switch (ty) {
    case IrType::I8: return "movsbq";
    case IrType::I16: return "movswq";
    case IrType::U16: return "movzwq";
    case IrType::I32: return "movslq";
    case IrType::U32:
    case IrType::F32: return "movl";
    default: return "movq";
  }
}

const char* X86Codegen::mov_store_for_type(IrType ty) const {
  switch (ty) {
    case IrType::I8: return "movb";
    case IrType::I16:
    case IrType::U16: return "movw";
    case IrType::I32:
    case IrType::U32:
    case IrType::F32: return "movl";
    default: return "movq";
  }
}

const char* X86Codegen::type_suffix(IrType ty) const {
  switch (ty) {
    case IrType::I8: return "b";
    case IrType::I16:
    case IrType::U16: return "w";
    case IrType::I32:
    case IrType::U32: return "l";
    default: return "q";
  }
}

void X86Codegen::emit_x86_atomic_op_loop(IrType ty, const char* op) {
  this->state.emit("    movq %rax, %r8");
  this->state.emit(std::string("    ") + this->mov_load_for_type(ty) + " (%rcx), " +
                   load_dest_reg(ty));

  const auto loop_label = atomic_loop_label();
  this->state.emit(loop_label + ":");
  this->state.emit("    movq %rax, %rdx");

  const auto size_suffix = this->type_suffix(ty);
  const auto rdx_reg = this->reg_for_type("rdx", ty);
  const char* r8_reg = "r8";
  switch (ty) {
    case IrType::I8: r8_reg = "r8b"; break;
    case IrType::I16:
    case IrType::U16: r8_reg = "r8w"; break;
    case IrType::I32:
    case IrType::U32: r8_reg = "r8d"; break;
    default: break;
  }

  if (std::string_view(op) == "sub") {
    this->state.emit(std::string("    sub") + size_suffix + " %" + r8_reg + ", %" + rdx_reg);
  } else if (std::string_view(op) == "and") {
    this->state.emit(std::string("    and") + size_suffix + " %" + r8_reg + ", %" + rdx_reg);
  } else if (std::string_view(op) == "or") {
    this->state.emit(std::string("    or") + size_suffix + " %" + r8_reg + ", %" + rdx_reg);
  } else if (std::string_view(op) == "xor") {
    this->state.emit(std::string("    xor") + size_suffix + " %" + r8_reg + ", %" + rdx_reg);
  } else if (std::string_view(op) == "nand") {
    this->state.emit(std::string("    and") + size_suffix + " %" + r8_reg + ", %" + rdx_reg);
    this->state.emit(std::string("    not") + size_suffix + " %" + rdx_reg);
  }

  this->state.emit(std::string("    lock cmpxchg") + size_suffix + " %" + rdx_reg + ", (%rcx)");
  this->state.emit("    jne " + loop_label);
}

}  // namespace c4c::backend::x86

namespace {

bool asm_symbol_char(char ch) {
  const auto uch = static_cast<unsigned char>(ch);
  return std::isalnum(uch) || ch == '_' || ch == '.' || ch == '$';
}

}  // namespace

namespace c4c::backend::x86::core {

void AssemblyTextBuffer::append_line(std::string_view line) {
  text_.append(line);
  text_.push_back('\n');
}

void AssemblyTextBuffer::append_raw(std::string_view text) {
  text_.append(text);
}

bool asm_text_references_symbol(std::string_view asm_text, std::string_view symbol_name) {
  if (symbol_name.empty()) {
    return false;
  }
  std::size_t search_from = 0;
  while (true) {
    const auto pos = asm_text.find(symbol_name, search_from);
    if (pos == std::string_view::npos) {
      return false;
    }
    const auto end = pos + symbol_name.size();
    const bool prev_is_symbol = pos > 0 && asm_symbol_char(asm_text[pos - 1]);
    const bool next_is_symbol = end < asm_text.size() && asm_symbol_char(asm_text[end]);
    if (!prev_is_symbol && !next_is_symbol) {
      return true;
    }
    search_from = pos + 1;
  }
}

bool asm_text_defines_symbol(std::string_view asm_text, std::string_view symbol_name) {
  if (symbol_name.empty()) {
    return false;
  }
  std::size_t search_from = 0;
  while (true) {
    const auto pos = asm_text.find(symbol_name, search_from);
    if (pos == std::string_view::npos) {
      return false;
    }
    const auto end = pos + symbol_name.size();
    const bool prev_is_symbol = pos > 0 && asm_symbol_char(asm_text[pos - 1]);
    const bool next_is_label = end < asm_text.size() && asm_text[end] == ':';
    if (!prev_is_symbol && next_is_label) {
      return true;
    }
    search_from = pos + 1;
  }
}

}  // namespace c4c::backend::x86::core
