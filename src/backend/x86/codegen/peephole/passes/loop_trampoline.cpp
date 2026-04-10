// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/loop_trampoline.rs
// Loop trampoline elimination pass.

#include "../peephole.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::x86::codegen::peephole::passes {

namespace {

void mark_nop(LineInfo& info) {
  info.kind = LineKind::Nop;
  info.dest_reg = REG_NONE;
}

std::string_view trimmed_line(const LineStore* store, const LineInfo& info,
                              std::size_t index) {
  return std::string_view(store->get(index)).substr(info.trim_start);
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

    const std::size_t jump_index = next_real_line(infos, *label_index + 1, len);
    if (jump_index >= len || infos[jump_index].kind != LineKind::Jmp) {
      continue;
    }

    const auto jump_line = trimmed_line(store, infos[jump_index], jump_index);
    const auto redirected_target = extract_jump_target(jump_line);
    if (!redirected_target.has_value() || *redirected_target == *branch_target) {
      continue;
    }

    replace_jump_target(store, infos[i], i, *redirected_target);
    mark_nop(infos[jump_index]);
    changed = true;
  }

  return changed;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
