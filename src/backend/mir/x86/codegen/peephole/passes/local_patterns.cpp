// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/local_patterns.rs
// Local peephole pattern matching passes.

#include "../peephole.hpp"

#include <algorithm>

namespace c4c::backend::x86::codegen::peephole::passes {

static std::string trimmed_line(const LineStore* store, const LineInfo& info, std::size_t index);
static void mark_nop(LineInfo& info);
static void replace_line(LineStore* store, LineInfo& info, std::size_t index, const std::string& text);
static std::size_t collect_non_nop_indices(const LineInfo* infos, std::size_t start, std::size_t len, std::size_t* out, std::size_t out_len);
static bool instruction_writes_to(std::string_view line, std::string_view reg);
static bool can_redirect_instruction(std::string_view line);
static std::optional<std::string> replace_dest_register(std::string_view line, std::string_view old_reg, std::string_view new_reg);
static std::optional<std::string> parse_setcc(std::string_view line);
static std::string invert_cc(std::string_view cc);
static bool is_redundant_zero_extend(std::string_view line, const LineInfo& info);

bool combined_local_pass(LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();
  bool rax_is_zero = false;

  for (std::size_t i = 0; i < len; ++i) {
    if (infos[i].kind == LineKind::Nop) {
      continue;
    }

    const auto line = trimmed_line(store, infos[i], i);

    if (rax_is_zero && infos[i].kind == LineKind::Other && infos[i].dest_reg == 0 && line == "xorl %eax, %eax") {
      mark_nop(infos[i]);
      changed = true;
      continue;
    }

    if (infos[i].kind == LineKind::SelfMove) {
      mark_nop(infos[i]);
      changed = true;
      continue;
    }

    if (infos[i].kind == LineKind::Other && infos[i].dest_reg == 0 && line == "xorl %eax, %eax") {
      rax_is_zero = true;
    } else if (infos[i].kind == LineKind::Other && infos[i].dest_reg == 0) {
      rax_is_zero = false;
    } else if (infos[i].kind == LineKind::Label || infos[i].kind == LineKind::Jmp ||
               infos[i].kind == LineKind::JmpIndirect || infos[i].kind == LineKind::CondJmp ||
               infos[i].kind == LineKind::Call || infos[i].kind == LineKind::Ret ||
               infos[i].kind == LineKind::Directive) {
      rax_is_zero = false;
    }

    if (infos[i].kind == LineKind::Other) {
      if (auto parsed = parse_reg_to_reg_movq(infos[i], line); parsed.has_value()) {
        const auto [src, dst] = *parsed;
        std::size_t j = i + 1;
        while (j < len && infos[j].kind == LineKind::Nop) {
          ++j;
        }
        if (j < len && infos[j].kind == LineKind::Other) {
          const auto next_line = trimmed_line(store, infos[j], j);
          if (auto reverse = parse_reg_to_reg_movq(infos[j], next_line); reverse.has_value()) {
            const auto [src2, dst2] = *reverse;
            if (src2 == dst && dst2 == src) {
              mark_nop(infos[j]);
              changed = true;
            }
          }
        }
      }
    }

    if (infos[i].kind == LineKind::Jmp) {
      const auto jmp_line = line;
      if (auto target = extract_jump_target(jmp_line); target.has_value()) {
        std::size_t j = i + 1;
        while (j < len && (infos[j].kind == LineKind::Nop || infos[j].kind == LineKind::Empty)) {
          ++j;
        }
        if (j < len && infos[j].kind == LineKind::Label) {
          const auto label_line = trimmed_line(store, infos[j], j);
          if (ends_with(label_line, ":") &&
              label_line.substr(0, label_line.size() - 1) == *target) {
            mark_nop(infos[i]);
            changed = true;
          }
        }
      }
    }

    if (infos[i].kind == LineKind::CondJmp) {
      std::size_t seq[4] = {};
      const auto count = collect_non_nop_indices(infos, i, len, seq, 4);
      if (count >= 4) {
        const auto cc_line = trimmed_line(store, infos[seq[0]], seq[0]);
        const auto set_line = trimmed_line(store, infos[seq[1]], seq[1]);
        const auto test_line = trimmed_line(store, infos[seq[2]], seq[2]);
        const auto jmp_line = trimmed_line(store, infos[seq[3]], seq[3]);
        if (auto cc = parse_setcc(set_line); cc.has_value() &&
            (test_line == "testq %rax, %rax" || test_line == "testl %eax, %eax") &&
            (starts_with(jmp_line, "jne ") || starts_with(jmp_line, "je "))) {
          const bool is_jne = starts_with(jmp_line, "jne ");
          const auto target = std::string(jmp_line.substr(4));
          const auto fused = std::string("    j") + (is_jne ? std::string(*cc) : invert_cc(*cc)) + " " + target;
          replace_line(store, infos[seq[3]], seq[3], fused);
          mark_nop(infos[seq[1]]);
          mark_nop(infos[seq[2]]);
          changed = true;
        }
      }
    }
  }

  return changed;
}

bool fuse_movq_ext_truncation(LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();
  for (std::size_t i = 0; i + 1 < len; ++i) {
    if (infos[i].kind != LineKind::Other || infos[i + 1].kind != LineKind::Other) {
      continue;
    }
    const auto a = trimmed_line(store, infos[i], i);
    const auto b = trimmed_line(store, infos[i + 1], i + 1);
    if (starts_with(a, "movq ") && b == "movl %eax, %eax") {
      mark_nop(infos[i + 1]);
      changed = true;
    }
  }
  return changed;
}

static std::string trimmed_line(const LineStore* store, const LineInfo& info, std::size_t index) {
  (void)index;
  return std::string(store->get(index).substr(info.trim_start));
}

static void mark_nop(LineInfo& info) {
  info.kind = LineKind::Nop;
  info.dest_reg = REG_NONE;
}

static void replace_line(LineStore* store, LineInfo& info, std::size_t index, const std::string& text) {
  store->get(index) = text;
  info.trim_start = 0;
}

static std::size_t collect_non_nop_indices(const LineInfo* infos, std::size_t start, std::size_t len, std::size_t* out, std::size_t out_len) {
  std::size_t count = 0;
  for (std::size_t i = start; i < len && count < out_len; ++i) {
    if (infos[i].kind == LineKind::Nop) {
      continue;
    }
    out[count++] = i;
  }
  return count;
}

static bool instruction_writes_to(std::string_view line, std::string_view reg) {
  return contains(line, reg);
}

static bool can_redirect_instruction(std::string_view line) {
  return starts_with(line, "mov") || starts_with(line, "lea");
}

static std::optional<std::string> replace_dest_register(std::string_view line, std::string_view old_reg, std::string_view new_reg) {
  const auto comma = line.rfind(',');
  if (comma == std::string_view::npos) {
    return std::nullopt;
  }
  auto out = std::string(line.substr(0, comma + 1));
  auto dst = std::string(line.substr(comma + 1));
  if (dst.find(old_reg) == std::string::npos) {
    return std::nullopt;
  }
  out += std::string(dst.replace(dst.find(old_reg), old_reg.size(), new_reg));
  return out;
}

static std::optional<std::string> parse_setcc(std::string_view line) {
  if (!starts_with(line, "set") || line.find(' ') == std::string_view::npos) {
    return std::nullopt;
  }
  const auto cc = line.substr(3, line.find(' ') - 3);
  return std::string(cc);
}

static std::string invert_cc(std::string_view cc) {
  if (cc == "e") return "ne";
  if (cc == "ne") return "e";
  if (cc == "l") return "ge";
  if (cc == "ge") return "l";
  if (cc == "g") return "le";
  if (cc == "le") return "g";
  if (cc == "b") return "ae";
  if (cc == "ae") return "b";
  return std::string(cc);
}

static bool is_redundant_zero_extend(std::string_view line, const LineInfo& info) {
  return info.kind == LineKind::Other && (line == "movl %eax, %eax" || line == "cltq");
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
