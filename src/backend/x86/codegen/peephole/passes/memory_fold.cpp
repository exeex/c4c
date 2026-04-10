// Mechanical C++-shaped translation of
// ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/memory_fold.rs
// Memory operand folding pass.

#include "../peephole.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace c4c::backend::x86::codegen::peephole::passes {

namespace {

void mark_nop(LineInfo& info) {
  info.kind = LineKind::Nop;
  info.dest_reg = REG_NONE;
  info.reg_refs = 0;
  info.rbp_offset = RBP_OFFSET_NONE;
  info.has_indirect_mem = false;
}

void replace_line(LineStore* store, LineInfo& info, std::size_t index,
                  const std::string& text) {
  store->get(index) = text;
  info = classify_line(store->get(index));
}

std::string_view trimmed_line(const LineStore* store, const LineInfo& info,
                              std::size_t index) {
  return std::string_view(store->get(index)).substr(info.trim_start);
}

std::string format_rbp_offset(std::int32_t offset) {
  return std::to_string(offset) + "(%rbp)";
}

bool is_foldable_op(std::string_view trimmed, std::size_t* mnemonic_len) {
  if (starts_with(trimmed, "add") || starts_with(trimmed, "sub") ||
      starts_with(trimmed, "and") || starts_with(trimmed, "xor") ||
      starts_with(trimmed, "cmp")) {
    *mnemonic_len = 3;
    return true;
  }
  if (starts_with(trimmed, "test")) {
    *mnemonic_len = 4;
    return true;
  }
  if (starts_with(trimmed, "or")) {
    *mnemonic_len = 2;
    return true;
  }
  return false;
}

bool parse_alu_reg_reg(std::string_view trimmed, std::string_view* op_with_suffix,
                       std::string_view* dst_str, RegId* src_fam,
                       RegId* dst_fam) {
  std::size_t mnemonic_len = 0;
  if (!is_foldable_op(trimmed, &mnemonic_len) || trimmed.size() <= mnemonic_len) {
    return false;
  }

  const char suffix = trimmed[mnemonic_len];
  if (suffix != 'q' && suffix != 'l' && suffix != 'w' && suffix != 'b') {
    return false;
  }

  *op_with_suffix = trimmed.substr(0, mnemonic_len + 1);
  const auto rest = trim_spaces(trimmed.substr(mnemonic_len + 1));
  const auto comma = rest.find(',');
  if (comma == std::string_view::npos) {
    return false;
  }

  const auto src_str = trim_spaces(rest.substr(0, comma));
  *dst_str = trim_spaces(rest.substr(comma + 1));
  if (!starts_with(src_str, "%") || !starts_with(*dst_str, "%")) {
    return false;
  }

  *src_fam = register_family_fast(src_str);
  *dst_fam = register_family_fast(*dst_str);
  return is_valid_gp_reg(*src_fam) && is_valid_gp_reg(*dst_fam);
}

}  // namespace

bool fold_memory_operands(LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();

  std::size_t i = 0;
  while (i + 1 < len) {
    if (infos[i].is_nop() || infos[i].kind != LineKind::LoadRbp ||
        infos[i].rbp_offset == RBP_OFFSET_NONE) {
      ++i;
      continue;
    }

    const RegId load_reg = infos[i].dest_reg;
    if (load_reg > 2) {
      ++i;
      continue;
    }

    const auto load_line = trimmed_line(store, infos[i], i);
    if (!starts_with(load_line, "movq ") && !starts_with(load_line, "movl ")) {
      ++i;
      continue;
    }

    std::size_t j = i + 1;
    while (j < len && (infos[j].is_nop() || infos[j].kind == LineKind::Empty)) {
      ++j;
    }
    if (j >= len) {
      ++i;
      continue;
    }

    if (infos[j].kind != LineKind::Other && infos[j].kind != LineKind::Cmp) {
      ++i;
      continue;
    }

    const auto folded_candidate = trimmed_line(store, infos[j], j);
    std::string_view op_with_suffix;
    std::string_view dst_str;
    RegId src_fam = REG_NONE;
    RegId dst_fam = REG_NONE;
    if (!parse_alu_reg_reg(folded_candidate, &op_with_suffix, &dst_str, &src_fam,
                           &dst_fam) ||
        src_fam != load_reg || dst_fam == load_reg) {
      ++i;
      continue;
    }

    const std::string new_inst =
        "    " + std::string(op_with_suffix) + " " +
        format_rbp_offset(infos[i].rbp_offset) + ", " + std::string(dst_str);
    mark_nop(infos[i]);
    replace_line(store, infos[j], j, new_inst);
    changed = true;
    i = j + 1;
  }

  return changed;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
