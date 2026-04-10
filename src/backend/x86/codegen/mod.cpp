#include "x86_codegen.hpp"

#include "../../regalloc.hpp"

#include <cctype>
#include <sstream>
#include <string_view>

namespace c4c::backend::x86 {

namespace {

[[maybe_unused]] constexpr std::string_view kCodegenModuleOverview =
    "Mechanical translation of ref/claudes-c-compiler/src/backend/x86/codegen";

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

std::vector<c4c::backend::PhysReg> x86_callee_saved_regs() {
  return {{1}, {2}, {3}, {4}, {5}};
}

std::vector<c4c::backend::PhysReg> x86_caller_saved_regs() {
  return {{10}, {11}, {12}, {13}, {14}, {15}};
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
