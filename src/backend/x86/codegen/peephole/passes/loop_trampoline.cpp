// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/loop_trampoline.rs
// Loop trampoline elimination pass.

#include "../peephole.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::x86::codegen::peephole::passes {

namespace {

constexpr const char* kReg64Names[16] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
};

void mark_nop(LineInfo& info) {
  info.kind = LineKind::Nop;
  info.dest_reg = REG_NONE;
}

void replace_line(LineStore* store, LineInfo& info, std::size_t index,
                  const std::string& text) {
  store->get(index) = text;
  info.trim_start = 0;
}

std::string_view trimmed_line(const LineStore* store, const LineInfo& info,
                              std::size_t index) {
  return std::string_view(store->get(index)).substr(info.trim_start);
}

std::string_view trim_spaces(std::string_view text) {
  while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) {
    text.remove_prefix(1);
  }
  while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) {
    text.remove_suffix(1);
  }
  return text;
}

std::size_t next_real_line(const LineInfo* infos, std::size_t start,
                           std::size_t len) {
  std::size_t index = start;
  while (index < len &&
         (infos[index].kind == LineKind::Nop || infos[index].kind == LineKind::Empty)) {
    ++index;
  }
  return index;
}

bool has_fallthrough_predecessor(const LineInfo* infos, std::size_t label_index) {
  if (label_index == 0) {
    return false;
  }

  std::size_t prev = label_index;
  while (prev > 0) {
    --prev;
    if (infos[prev].kind == LineKind::Nop || infos[prev].kind == LineKind::Empty) {
      continue;
    }
    return !(infos[prev].kind == LineKind::Label || infos[prev].kind == LineKind::Directive ||
             infos[prev].is_barrier());
  }

  return false;
}

bool is_movq_reg_reg(std::string_view trimmed, std::string_view first_reg,
                     std::string_view second_reg) {
  if (!starts_with(trimmed, "movq ")) {
    return false;
  }
  const auto expected_len = 5 + first_reg.size() + 2 + second_reg.size();
  return trimmed.size() == expected_len &&
         trimmed.substr(5, first_reg.size()) == first_reg &&
         trimmed.substr(5 + first_reg.size(), 2) == ", " &&
         trimmed.substr(5 + first_reg.size() + 2) == second_reg;
}

RegId effective_dest_reg(const LineInfo& info, std::string_view trimmed) {
  if (info.dest_reg != REG_NONE) {
    return info.dest_reg;
  }
  const auto comma = trimmed.rfind(',');
  if (comma == std::string_view::npos) {
    return REG_NONE;
  }
  return register_family_fast(trim_spaces(trimmed.substr(comma + 1)));
}

std::optional<std::pair<RegId, RegId>> parse_reg_to_reg_movq_local(std::string_view trimmed) {
  if (!starts_with(trimmed, "movq ")) {
    return std::nullopt;
  }
  const auto rest = trimmed.substr(5);
  const auto comma = rest.find(',');
  if (comma == std::string_view::npos) {
    return std::nullopt;
  }
  const auto src = register_family_fast(trim_spaces(rest.substr(0, comma)));
  const auto dst = register_family_fast(trim_spaces(rest.substr(comma + 1)));
  if (!is_valid_gp_reg(src) || !is_valid_gp_reg(dst) || src == dst) {
    return std::nullopt;
  }
  return std::pair<RegId, RegId>{src, dst};
}

std::size_t count_branches_to_label(const LineStore* store, const LineInfo* infos,
                                    std::string_view label_name) {
  std::size_t count = 0;
  for (std::size_t i = 0; i < store->len(); ++i) {
    if (infos[i].kind != LineKind::Jmp && infos[i].kind != LineKind::CondJmp) {
      continue;
    }

    const auto line = trimmed_line(store, infos[i], i);
    const auto target = extract_jump_target(line);
    if (!target.has_value() || *target != label_name) {
      continue;
    }

    ++count;
  }

  return count;
}

std::optional<std::size_t> find_label_index(const LineStore* store, const LineInfo* infos,
                                            std::string_view label_name) {
  for (std::size_t i = 0; i < store->len(); ++i) {
    if (infos[i].kind != LineKind::Label) {
      continue;
    }

    const auto line = trimmed_line(store, infos[i], i);
    if (!ends_with(line, ":")) {
      continue;
    }
    if (line.substr(0, line.size() - 1) == label_name) {
      return i;
    }
  }

  return std::nullopt;
}

std::optional<std::vector<std::pair<RegId, RegId>>> collect_trampoline_moves(
    const LineStore* store, const LineInfo* infos, std::size_t label_index,
    std::size_t len, std::size_t* jump_index) {
  std::vector<std::pair<RegId, RegId>> moves;
  std::size_t index = next_real_line(infos, label_index + 1, len);
  while (index < len) {
    if (infos[index].kind == LineKind::Jmp) {
      *jump_index = index;
      return moves;
    }
    if (infos[index].kind != LineKind::Other) {
      return std::nullopt;
    }
    const auto trimmed = trimmed_line(store, infos[index], index);
    const auto move = parse_reg_to_reg_movq_local(trimmed);
    if (!move.has_value()) {
      return std::nullopt;
    }
    moves.push_back(*move);
    index = next_real_line(infos, index + 1, len);
  }

  return std::nullopt;
}

bool writes_reg_without_read(const LineStore* store, const LineInfo& info,
                            std::size_t index, RegId reg) {
  const auto dest_reg = effective_dest_reg(info, trimmed_line(store, info, index));
  if (dest_reg != reg) {
    return false;
  }
  switch (info.kind) {
    case LineKind::LoadRbp:
    case LineKind::Pop:
    case LineKind::SetCC:
      return true;
    case LineKind::Other:
      return !is_read_modify_write(trimmed_line(store, info, index));
    default:
      return false;
  }
}

bool verify_fallthrough_safety(const LineStore* store, const LineInfo* infos,
                               std::size_t branch_index, std::size_t len,
                               RegId src_fam, RegId dst_fam) {
  bool killed_src = false;
  bool killed_dst = false;
  std::uint32_t jumps_followed = 0;

  for (std::size_t index = branch_index + 1; index < len; ++index) {
    if (infos[index].kind == LineKind::Nop || infos[index].kind == LineKind::Empty ||
        infos[index].kind == LineKind::Label) {
      continue;
    }

    if (infos[index].kind == LineKind::Jmp) {
      if (jumps_followed >= 2 || (killed_src && killed_dst)) {
        break;
      }
      const auto target = extract_jump_target(trimmed_line(store, infos[index], index));
      if (!target.has_value()) {
        break;
      }
      const auto jump_label_index = find_label_index(store, infos, *target);
      if (!jump_label_index.has_value()) {
        break;
      }
      ++jumps_followed;
      index = *jump_label_index;
      continue;
    }

    if (infos[index].kind == LineKind::JmpIndirect || infos[index].kind == LineKind::Ret ||
        infos[index].kind == LineKind::CondJmp) {
      break;
    }

    const bool kills_src = writes_reg_without_read(store, infos[index], index, src_fam);
    const bool kills_dst = writes_reg_without_read(store, infos[index], index, dst_fam);

    if (!killed_src && line_references_reg_fast(infos[index], src_fam) && !kills_src) {
      return false;
    }
    if (!killed_dst && line_references_reg_fast(infos[index], dst_fam) && !kills_dst) {
      return false;
    }

    if (!killed_src && kills_src) {
      killed_src = true;
    }
    if (!killed_dst && kills_dst) {
      killed_dst = true;
    }

    if (killed_src && killed_dst) {
      break;
    }
  }

  return true;
}

bool find_copy_and_modifications(const LineStore* store, const LineInfo* infos,
                                 std::size_t branch_index, RegId src_fam,
                                 RegId dst_fam, std::size_t* copy_index,
                                 std::vector<std::size_t>* modifications) {
  const std::string src_reg = std::string("%") + kReg64Names[src_fam];
  const std::string dst_reg = std::string("%") + kReg64Names[dst_fam];

  for (std::size_t scan = branch_index; scan > 0;) {
    --scan;
    if (infos[scan].kind == LineKind::Nop || infos[scan].kind == LineKind::Empty) {
      continue;
    }
    if (infos[scan].kind == LineKind::Label || infos[scan].kind == LineKind::Call ||
        infos[scan].kind == LineKind::Jmp || infos[scan].kind == LineKind::JmpIndirect ||
        infos[scan].kind == LineKind::Ret || infos[scan].kind == LineKind::Directive) {
      break;
    }

    const auto trimmed = trimmed_line(store, infos[scan], scan);
    const bool writes_src = effective_dest_reg(infos[scan], trimmed) == src_fam;
    if (writes_src) {
      if (infos[scan].kind == LineKind::SetCC) {
        return false;
      }
      if (is_movq_reg_reg(trimmed, dst_reg, src_reg)) {
        *copy_index = scan;
        return true;
      }
      modifications->push_back(scan);
      continue;
    }

    const bool refs_src = line_references_reg_fast(infos[scan], src_fam);
    const bool refs_dst = line_references_reg_fast(infos[scan], dst_fam);
    if (refs_src) {
      if (refs_dst) {
        return false;
      }
      modifications->push_back(scan);
      continue;
    }
    if (refs_dst) {
      return false;
    }
  }

  return false;
}

void replace_jump_target(LineStore* store, const LineInfo& info, std::size_t index,
                         std::string_view new_target) {
  const auto line = trimmed_line(store, info, index);
  const auto old_target = extract_jump_target(line);
  if (!old_target.has_value()) {
    return;
  }

  auto rendered = store->get(index);
  const auto target_pos = rendered.rfind(std::string(*old_target));
  if (target_pos == std::string::npos) {
    return;
  }
  rendered.replace(target_pos, old_target->size(), new_target);
  store->get(index) = std::move(rendered);
}

}  // namespace

bool eliminate_loop_trampolines(LineStore* store, LineInfo* infos) {
  const std::size_t len = store->len();
  bool changed = false;

  for (std::size_t i = 0; i < len; ++i) {
    if (infos[i].kind != LineKind::Jmp && infos[i].kind != LineKind::CondJmp) {
      continue;
    }

    const auto branch_line = trimmed_line(store, infos[i], i);
    const auto branch_target = extract_jump_target(branch_line);
    if (!branch_target.has_value()) {
      continue;
    }

    const auto label_index = find_label_index(store, infos, *branch_target);
    if (!label_index.has_value()) {
      continue;
    }
    if (count_branches_to_label(store, infos, *branch_target) != 1) {
      continue;
    }
    if (has_fallthrough_predecessor(infos, *label_index)) {
      continue;
    }

    std::size_t jump_index = len;
    const auto trampoline_moves =
        collect_trampoline_moves(store, infos, *label_index, len, &jump_index);
    if (!trampoline_moves.has_value() || jump_index >= len) {
      continue;
    }

    const auto jump_line = trimmed_line(store, infos[jump_index], jump_index);
    const auto redirected_target = extract_jump_target(jump_line);
    if (!redirected_target.has_value() || *redirected_target == *branch_target) {
      continue;
    }

    std::vector<std::size_t> copy_indices;
    std::vector<std::size_t> trampoline_move_indices;
    std::vector<std::pair<std::size_t, std::string>> rewrites;
    bool can_coalesce = true;

    std::size_t move_scan = next_real_line(infos, *label_index + 1, len);
    for (const auto& [src_fam, dst_fam] : *trampoline_moves) {
      if (!verify_fallthrough_safety(store, infos, i, len, src_fam, dst_fam)) {
        can_coalesce = false;
        break;
      }

      std::size_t copy_index = len;
      std::vector<std::size_t> modifications;
      if (!find_copy_and_modifications(store, infos, i, src_fam, dst_fam, &copy_index,
                                       &modifications)) {
        can_coalesce = false;
        break;
      }

      copy_indices.push_back(copy_index);
      trampoline_move_indices.push_back(move_scan);
      for (const auto mod_index : modifications) {
        const auto rewritten =
            replace_reg_family(trimmed_line(store, infos[mod_index], mod_index), src_fam, dst_fam);
        if (rewritten == trimmed_line(store, infos[mod_index], mod_index)) {
          can_coalesce = false;
          break;
        }
        rewrites.emplace_back(mod_index, "  " + rewritten);
      }
      if (!can_coalesce) {
        break;
      }
      move_scan = next_real_line(infos, move_scan + 1, len);
    }

    if (!can_coalesce) {
      continue;
    }

    for (const auto& [mod_index, rewritten] : rewrites) {
      replace_line(store, infos[mod_index], mod_index, rewritten);
    }
    for (const auto copy_index : copy_indices) {
      mark_nop(infos[copy_index]);
    }
    for (const auto move_index : trampoline_move_indices) {
      mark_nop(infos[move_index]);
    }

    replace_jump_target(store, infos[i], i, *redirected_target);
    mark_nop(infos[jump_index]);
    changed = true;
  }

  return changed;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
