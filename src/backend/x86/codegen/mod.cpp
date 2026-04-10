#include "x86_codegen.hpp"

#include "../../generation.hpp"
#include "../../regalloc.hpp"
#include "../../bir.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>

namespace c4c::backend::x86 {

namespace {

[[maybe_unused]] constexpr std::string_view kCodegenModuleOverview =
    "Mechanical translation of ref/claudes-c-compiler/src/backend/x86/codegen";
constexpr std::int64_t kX86StackProbePageSize = 4096;
constexpr std::int64_t kX86ParamStackBase = 16;
constexpr const char* kX86ArgRegs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

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

std::string x86_reg_name_to_64(std::string_view reg) {
  if (reg == "rax" || reg == "eax" || reg == "ax" || reg == "al" || reg == "ah") return "rax";
  if (reg == "rbx" || reg == "ebx" || reg == "bx" || reg == "bl" || reg == "bh") return "rbx";
  if (reg == "rcx" || reg == "ecx" || reg == "cx" || reg == "cl" || reg == "ch") return "rcx";
  if (reg == "rdx" || reg == "edx" || reg == "dx" || reg == "dl" || reg == "dh") return "rdx";
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

std::string x86_reg_name_to_16(std::string_view reg) {
  const auto wide = x86_reg_name_to_64(reg);
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

std::string x86_reg_name_to_8l(std::string_view reg) {
  const auto wide = x86_reg_name_to_64(reg);
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

std::string x86_reg_name_to_8h(std::string_view reg) {
  const auto wide = x86_reg_name_to_64(reg);
  if (wide == "rax") return "ah";
  if (wide == "rbx") return "bh";
  if (wide == "rcx") return "ch";
  if (wide == "rdx") return "dh";
  return std::string(reg);
}

std::string x86_format_reg(std::string_view reg, std::optional<char> modifier) {
  if ((reg.size() >= 3 && reg.substr(0, 3) == "xmm") || reg == "st" ||
      (reg.size() >= 3 && reg.substr(0, 3) == "st(")) {
    return std::string(reg);
  }
  switch (modifier.value_or('q')) {
    case 'k':
    case 'l': {
      const char* mapped = reg_name_to_32(x86_reg_name_to_64(reg));
      return *mapped == '\0' ? std::string(reg) : std::string(mapped);
    }
    case 'w': return x86_reg_name_to_16(reg);
    case 'b': return x86_reg_name_to_8l(reg);
    case 'h': return x86_reg_name_to_8h(reg);
    case 'q': return x86_reg_name_to_64(reg);
    default: return std::string(reg);
  }
}

const char* x86_gcc_cc_to_x86(std::string_view cond) {
  if (cond == "e" || cond == "z") return "e";
  if (cond == "ne" || cond == "nz") return "ne";
  if (cond == "s") return "s";
  if (cond == "ns") return "ns";
  if (cond == "g") return "g";
  if (cond == "ge") return "ge";
  if (cond == "l") return "l";
  if (cond == "le") return "le";
  if (cond == "a") return "a";
  if (cond == "ae") return "ae";
  if (cond == "b" || cond == "c") return "b";
  if (cond == "be") return "be";
  if (cond == "o") return "o";
  if (cond == "no") return "no";
  if (cond == "p") return "p";
  if (cond == "np") return "np";
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

const char* phys_reg_name_32(c4c::backend::PhysReg reg) {
  switch (reg.index) {
    case 1: return "ebx";
    case 2: return "r12d";
    case 3: return "r13d";
    case 4: return "r14d";
    case 5: return "r15d";
    case 10: return "r11d";
    case 11: return "r10d";
    case 12: return "r8d";
    case 13: return "r9d";
    case 14: return "edi";
    case 15: return "esi";
    default: return "";
  }
}

const char* x86_alu_mnemonic(c4c::backend::bir::BinaryOpcode op) {
  switch (op) {
    case c4c::backend::bir::BinaryOpcode::Add: return "add";
    case c4c::backend::bir::BinaryOpcode::Sub: return "sub";
    case c4c::backend::bir::BinaryOpcode::And: return "and";
    case c4c::backend::bir::BinaryOpcode::Or: return "or";
    case c4c::backend::bir::BinaryOpcode::Xor: return "xor";
    default: return "add";
  }
}

std::pair<const char*, const char*> x86_shift_mnemonic(c4c::backend::bir::BinaryOpcode op) {
  switch (op) {
    case c4c::backend::bir::BinaryOpcode::Shl: return {"shll", "shlq"};
    case c4c::backend::bir::BinaryOpcode::AShr: return {"sarl", "sarq"};
    case c4c::backend::bir::BinaryOpcode::LShr: return {"shrl", "shrq"};
    default: return {"shll", "shlq"};
  }
}

std::vector<c4c::backend::PhysReg> x86_callee_saved_regs() {
  return {{1}, {2}, {3}, {4}, {5}};
}

std::vector<c4c::backend::PhysReg> x86_caller_saved_regs() {
  return {{10}, {11}, {12}, {13}, {14}, {15}};
}

std::vector<c4c::backend::PhysReg> x86_prune_caller_saved_regs(bool has_indirect_call,
                                                               bool has_i128_ops,
                                                               bool has_atomic_rmw) {
  auto caller_saved = x86_caller_saved_regs();
  const auto drop_reg = [&](std::uint32_t index) {
    caller_saved.erase(
        std::remove_if(caller_saved.begin(),
                       caller_saved.end(),
                       [&](const c4c::backend::PhysReg& reg) { return reg.index == index; }),
        caller_saved.end());
  };

  if (has_indirect_call) {
    drop_reg(11);  // r10
  }
  if (has_i128_ops) {
    drop_reg(12);  // r8
    drop_reg(13);  // r9
    drop_reg(14);  // rdi
    drop_reg(15);  // rsi
  }
  if (has_atomic_rmw) {
    drop_reg(12);  // r8
  }

  return caller_saved;
}

std::optional<c4c::backend::PhysReg> x86_constraint_to_callee_saved(std::string_view constraint) {
  if (constraint.size() >= 3 && constraint.front() == '{' && constraint.back() == '}') {
    const auto reg = constraint.substr(1, constraint.size() - 2);
    if (reg == "rbx" || reg == "ebx" || reg == "bx" || reg == "bl" || reg == "bh") {
      return c4c::backend::PhysReg{1};
    }
    if (reg == "r12" || reg == "r12d" || reg == "r12w" || reg == "r12b") {
      return c4c::backend::PhysReg{2};
    }
    if (reg == "r13" || reg == "r13d" || reg == "r13w" || reg == "r13b") {
      return c4c::backend::PhysReg{3};
    }
    if (reg == "r14" || reg == "r14d" || reg == "r14w" || reg == "r14b") {
      return c4c::backend::PhysReg{4};
    }
    if (reg == "r15" || reg == "r15d" || reg == "r15w" || reg == "r15b") {
      return c4c::backend::PhysReg{5};
    }
    return std::nullopt;
  }

  for (const char ch : constraint) {
    if (ch == 'b') {
      return c4c::backend::PhysReg{1};
    }
  }
  return std::nullopt;
}

std::optional<c4c::backend::PhysReg> x86_clobber_name_to_callee_saved(std::string_view name) {
  if (name == "rbx" || name == "ebx" || name == "bx" || name == "bl" || name == "bh") {
    return c4c::backend::PhysReg{1};
  }
  if (name == "r12" || name == "r12d" || name == "r12w" || name == "r12b") {
    return c4c::backend::PhysReg{2};
  }
  if (name == "r13" || name == "r13d" || name == "r13w" || name == "r13b") {
    return c4c::backend::PhysReg{3};
  }
  if (name == "r14" || name == "r14d" || name == "r14w" || name == "r14b") {
    return c4c::backend::PhysReg{4};
  }
  if (name == "r15" || name == "r15d" || name == "r15w" || name == "r15b") {
    return c4c::backend::PhysReg{5};
  }
  return std::nullopt;
}

const char* x86_arg_reg_name(std::size_t reg_index) {
  return reg_index < std::size(kX86ArgRegs) ? kX86ArgRegs[reg_index] : "";
}

std::int64_t x86_param_stack_base_offset() { return kX86ParamStackBase; }

std::int64_t x86_param_stack_offset(std::int64_t class_stack_offset) {
  return x86_param_stack_base_offset() + class_stack_offset;
}

const char* x86_param_ref_scalar_load_instr(std::string_view scalar_type) {
  if (scalar_type == "i32") {
    return "movslq";
  }
  if (scalar_type == "u32" || scalar_type == "f32") {
    return "movl";
  }
  if (scalar_type == "i64" || scalar_type == "u64" || scalar_type == "f64" ||
      scalar_type == "ptr") {
    return "movq";
  }
  return "";
}

const char* x86_param_ref_scalar_dest_reg(std::string_view scalar_type) {
  if (scalar_type == "u32" || scalar_type == "f32") {
    return "%eax";
  }
  if (scalar_type == "i32" || scalar_type == "i64" || scalar_type == "u64" ||
      scalar_type == "f64" || scalar_type == "ptr") {
    return "%rax";
  }
  return "";
}

const char* x86_param_ref_scalar_arg_reg(std::size_t reg_index, std::string_view scalar_type) {
  if (reg_index >= std::size(kX86ArgRegs)) {
    return "";
  }
  const auto base_reg = x86_arg_reg_name(reg_index);
  if (scalar_type == "i32" || scalar_type == "u32" || scalar_type == "f32") {
    return reg_name_to_32(base_reg);
  }
  if (scalar_type == "i64" || scalar_type == "u64" || scalar_type == "f64" ||
      scalar_type == "ptr") {
    return base_reg;
  }
  return "";
}

std::string x86_param_ref_scalar_stack_operand(std::int64_t class_stack_offset,
                                               std::string_view scalar_type) {
  const auto stack_offset = x86_param_stack_offset(class_stack_offset);
  if (scalar_type == "i32" || scalar_type == "u32" || scalar_type == "f32") {
    return "DWORD PTR [rbp + " + std::to_string(stack_offset) + "]";
  }
  if (scalar_type == "i64" || scalar_type == "u64" || scalar_type == "f64" ||
      scalar_type == "ptr") {
    return "QWORD PTR [rbp + " + std::to_string(stack_offset) + "]";
  }
  return "";
}

bool x86_phys_reg_is_callee_saved(c4c::backend::PhysReg reg) {
  return reg.index >= 1 && reg.index <= 5;
}

bool x86_param_can_prestore_direct_to_reg(bool has_stack_slot,
                                          std::optional<c4c::backend::PhysReg> assigned_reg,
                                          std::size_t assigned_param_count) {
  return !has_stack_slot && assigned_reg.has_value() &&
         x86_phys_reg_is_callee_saved(*assigned_reg) && assigned_param_count == 1;
}

std::int64_t x86_variadic_reg_save_area_size(bool no_sse) { return no_sse ? 48 : 176; }

std::int64_t x86_aligned_frame_size(std::int64_t raw_space) {
  return raw_space > 0 ? (raw_space + 15) & ~15 : 0;
}

std::int64_t x86_stack_probe_page_size() { return kX86StackProbePageSize; }

bool x86_needs_stack_probe(std::int64_t frame_size) {
  return frame_size > x86_stack_probe_page_size();
}

std::int64_t x86_callee_saved_slot_offset(std::int64_t frame_size, std::size_t save_index) {
  return -frame_size + static_cast<std::int64_t>(save_index) * 8;
}

std::int64_t x86_variadic_gp_save_offset(std::int64_t reg_save_area_base, std::size_t reg_index) {
  return reg_save_area_base + static_cast<std::int64_t>(reg_index) * 8;
}

std::int64_t x86_variadic_sse_save_offset(std::int64_t reg_save_area_base, std::size_t reg_index) {
  return reg_save_area_base + 48 + static_cast<std::int64_t>(reg_index) * 16;
}

c4c::backend::RegAllocIntegrationResult run_shared_x86_regalloc(
    const c4c::backend::LivenessInput& liveness_input) {
  c4c::backend::RegAllocConfig config;
  config.available_regs = x86_callee_saved_regs();
  config.caller_saved_regs = x86_caller_saved_regs();
  return c4c::backend::run_regalloc_and_merge_clobbers(liveness_input, config, {});
}

std::string decode_llvm_byte_string(std::string_view text) {
  std::string bytes;
  bytes.reserve(text.size());
  for (std::size_t index = 0; index < text.size(); ++index) {
    if (text[index] == '\\' && index + 2 < text.size()) {
      auto hex_val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
      };
      const int hi = hex_val(text[index + 1]);
      const int lo = hex_val(text[index + 2]);
      if (hi >= 0 && lo >= 0) {
        bytes.push_back(static_cast<char>(hi * 16 + lo));
        index += 2;
        continue;
      }
    }
    bytes.push_back(text[index]);
  }
  return bytes;
}

std::string escape_asm_string(std::string_view raw_bytes) {
  std::ostringstream out;
  for (unsigned char ch : raw_bytes) {
    switch (ch) {
      case '\\': out << "\\\\"; break;
      case '"': out << "\\\""; break;
      case '\n': out << "\\n"; break;
      case '\t': out << "\\t"; break;
      default:
        if (std::isprint(ch) != 0) {
          out << static_cast<char>(ch);
        } else {
          constexpr char kHex[] = "0123456789ABCDEF";
          out << '\\' << kHex[(ch >> 4) & 0xF] << kHex[ch & 0xF];
        }
        break;
    }
  }
  return out.str();
}

std::string asm_symbol_name(std::string_view target_triple, std::string_view logical_name) {
  if (target_triple.find("apple-darwin") != std::string::npos) {
    return "_" + std::string(logical_name);
  }
  return std::string(logical_name);
}

std::string asm_private_data_label(std::string_view target_triple, std::string_view pool_name) {
  std::string label(pool_name);
  if (!label.empty() && label.front() == '@') {
    label.erase(label.begin());
  }
  while (!label.empty() && label.front() == '.') {
    label.erase(label.begin());
  }

  if (target_triple.find("apple-darwin") != std::string::npos) {
    return "L" + label;
  }
  return ".L." + label;
}

std::string emit_function_prelude(std::string_view target_triple, std::string_view symbol_name) {
  std::ostringstream out;
  out << ".globl " << symbol_name << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".type " << symbol_name << ", @function\n";
  }
  out << symbol_name << ":\n";
  return out.str();
}

std::string emit_global_symbol_prelude(std::string_view target_triple,
                                       std::string_view symbol_name,
                                       std::size_t align_bytes,
                                       bool is_zero_init) {
  std::ostringstream out;
  out << (is_zero_init ? ".bss\n" : ".data\n") << ".globl " << symbol_name << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".type " << symbol_name << ", @object\n";
  }
  if (align_bytes > 1) {
    out << ".p2align " << (align_bytes == 2 ? 1 : 2) << "\n";
  }
  out << symbol_name << ":\n";
  return out.str();
}

void x86_codegen_module_anchor() {
  // The real implementation is split across alu/calls/memory/comparison and
  // the other target-local translation units in this directory.
}

}  // namespace c4c::backend::x86
