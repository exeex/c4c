// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/push_pop.rs
// Push/pop pair elimination passes.

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

namespace c4c::backend::x86::codegen::peephole::passes {

struct LineInfo;
struct LineStore;

using RegId = std::uint8_t;
constexpr RegId REG_NONE = 255;

enum class LineKind {
  Nop,
  Empty,
  StoreRbp,
  LoadRbp,
  SelfMove,
  Label,
  Jmp,
  JmpIndirect,
  CondJmp,
  Call,
  Ret,
  Push,
  Pop,
  SetCC,
  Cmp,
  Directive,
  Other,
};

bool instruction_modifies_reg_id(const LineInfo& info, RegId reg_id);
bool instruction_modifies_stack(std::string_view line, const LineInfo& info);
bool parse_reg_to_reg_move(std::string_view line, std::string_view push_reg);
bool instruction_writes_to(std::string_view line, std::string_view reg);
bool can_redirect_instruction(std::string_view line);
std::string replace_dest_register(std::string_view line, std::string_view old_reg, std::string_view new_reg);
std::size_t collect_non_nop_indices(const LineInfo* infos, std::size_t start, std::size_t len, std::size_t* out, std::size_t out_len);
void mark_nop(LineInfo& info);
void replace_line(LineStore* store, LineInfo& info, std::size_t index, const std::string& text);

bool eliminate_push_pop_pairs(const LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();

  for (std::size_t i = 0; i + 2 < len; ++i) {
    if (infos[i].kind != LineKind::Push) {
      continue;
    }
    const RegId push_reg_id = infos[i].dest_reg;
    if (push_reg_id == REG_NONE) {
      continue;
    }

    for (std::size_t j = i + 1; j < std::min(i + 4, len); ++j) {
      if (infos[j].kind == LineKind::Nop) {
        continue;
      }
      if (infos[j].kind == LineKind::Pop && infos[j].dest_reg == push_reg_id) {
        bool safe = true;
        for (std::size_t k = i + 1; k < j; ++k) {
          if (infos[k].kind == LineKind::Nop) {
            continue;
          }
          if (instruction_modifies_reg_id(infos[k], push_reg_id)) {
            safe = false;
            break;
          }
          if (instruction_modifies_stack(store->get(k).substr(infos[k].trim_start), infos[k])) {
            safe = false;
            break;
          }
        }
        if (safe) {
          mark_nop(infos[i]);
          mark_nop(infos[j]);
          changed = true;
        }
        break;
      }
      if (infos[j].kind == LineKind::Push || infos[j].kind == LineKind::Call ||
          infos[j].kind == LineKind::Jmp || infos[j].kind == LineKind::JmpIndirect ||
          infos[j].kind == LineKind::Ret) {
        break;
      }
    }
  }

  return changed;
}

bool eliminate_binop_push_pop_pattern(LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();

  for (std::size_t i = 0; i + 3 < len; ++i) {
    if (infos[i].kind != LineKind::Push || infos[i].dest_reg == REG_NONE) {
      continue;
    }

    std::size_t real_indices[3] = {};
    const std::size_t count = collect_non_nop_indices(infos, i, len, real_indices, 3);
    if (count != 3) {
      continue;
    }

    const std::size_t load_idx = real_indices[0];
    const std::size_t move_idx = real_indices[1];
    const std::size_t pop_idx = real_indices[2];

    if (infos[pop_idx].kind != LineKind::Pop || infos[pop_idx].dest_reg != infos[i].dest_reg) {
      continue;
    }

    bool stack_safe = true;
    for (std::size_t k = i + 1; k <= pop_idx; ++k) {
      if (infos[k].kind != LineKind::Nop &&
          instruction_modifies_stack(store->get(k).substr(infos[k].trim_start), infos[k])) {
        stack_safe = false;
        break;
      }
    }
    if (!stack_safe) {
      continue;
    }

    const auto push_line = store->get(i).substr(infos[i].trim_start);
    const auto load_line = store->get(load_idx).substr(infos[load_idx].trim_start);
    const auto move_line = store->get(move_idx).substr(infos[move_idx].trim_start);
    const auto push_reg = push_line.substr(push_line.find(' ') + 1);

    if (parse_reg_to_reg_move(move_line, push_reg)) {
      if (instruction_writes_to(load_line, push_reg) && can_redirect_instruction(load_line)) {
        const auto new_load = replace_dest_register(load_line, push_reg, move_line.substr(move_line.rfind(',') + 1));
        mark_nop(infos[i]);
        replace_line(store, infos[load_idx], load_idx, "    " + new_load);
        mark_nop(infos[move_idx]);
        mark_nop(infos[pop_idx]);
        changed = true;
      }
    }
  }

  return changed;
}

bool instruction_modifies_stack(std::string_view line, const LineInfo& info) {
  if (line.empty()) {
    return false;
  }
  if (line.starts_with("pushf") || line.starts_with("popf")) {
    return true;
  }
  if ((line.starts_with("subq ") || line.starts_with("addq ")) && line.ends_with("%rsp")) {
    return true;
  }
  return info.kind == LineKind::Other && line.find("%rsp") != std::string_view::npos;
}

bool instruction_modifies_reg_id(const LineInfo& info, RegId reg_id) {
  switch (info.kind) {
    case LineKind::StoreRbp:
    case LineKind::Cmp:
    case LineKind::Nop:
    case LineKind::Empty:
    case LineKind::Label:
    case LineKind::Directive:
    case LineKind::Jmp:
    case LineKind::JmpIndirect:
    case LineKind::CondJmp:
    case LineKind::SelfMove:
      return false;
    case LineKind::LoadRbp:
    case LineKind::Pop:
      return info.dest_reg == reg_id;
    case LineKind::Push:
      return false;
    case LineKind::SetCC:
      return info.dest_reg == reg_id;
    case LineKind::Call:
      return true;
    case LineKind::Ret:
      return false;
    case LineKind::Other:
      return info.dest_reg == reg_id;
  }
  return false;
}

bool parse_reg_to_reg_move(std::string_view line, std::string_view push_reg) {
  return line.starts_with("movq ") && line.find(push_reg) != std::string_view::npos;
}

bool instruction_writes_to(std::string_view line, std::string_view reg) {
  return line.find(reg) != std::string_view::npos;
}

bool can_redirect_instruction(std::string_view line) {
  return line.starts_with("mov") || line.starts_with("lea");
}

std::string replace_dest_register(std::string_view line, std::string_view old_reg, std::string_view new_reg) {
  auto out = std::string(line);
  const auto pos = out.rfind(old_reg);
  if (pos != std::string::npos) {
    out.replace(pos, old_reg.size(), new_reg);
  }
  return out;
}

std::size_t collect_non_nop_indices(const LineInfo* infos, std::size_t start, std::size_t len, std::size_t* out, std::size_t out_len) {
  std::size_t count = 0;
  for (std::size_t i = start + 1; i < len && count < out_len; ++i) {
    if (infos[i].kind == LineKind::Nop) {
      continue;
    }
    out[count++] = i;
  }
  return count;
}

void mark_nop(LineInfo& info) { info.kind = LineKind::Nop; }

void replace_line(LineStore* store, LineInfo& info, std::size_t index, const std::string& text) {
  store->get(index) = text;
  info.trim_start = 0;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
