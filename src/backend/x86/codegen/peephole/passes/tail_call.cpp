// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/tail_call.rs
// Tail call optimization pass.

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

bool is_tail_call_candidate(const LineStore* store, const LineInfo* infos,
                            std::size_t call_idx, std::size_t len,
                            std::size_t* ret_idx) {
  const std::size_t limit = std::min(call_idx + 30, len);
  bool found_frame_teardown = false;
  bool found_pop_rbp = false;
  for (std::size_t j = call_idx + 1; j < limit; ++j) {
    if (infos[j].kind == LineKind::Nop || infos[j].kind == LineKind::Empty || infos[j].kind == LineKind::Directive) {
      continue;
    }
    if (infos[j].kind == LineKind::LoadRbp) {
      if (infos[j].dest_reg == 0) {
        return false;
      }
      continue;
    }
    if (infos[j].kind == LineKind::Other) {
      const auto line = trimmed_line(store, infos[j], j);
      if (line == "movq %rbp, %rsp") {
        found_frame_teardown = true;
        continue;
      }
      if (infos[j].dest_reg == 0) {
        return false;
      }
      return false;
    }
    if (infos[j].kind == LineKind::Pop) {
      if (infos[j].dest_reg == 5) {
        found_pop_rbp = true;
        continue;
      }
      return false;
    }
    if (infos[j].kind == LineKind::Ret) {
      if (found_frame_teardown && found_pop_rbp) {
        *ret_idx = j;
        return true;
      }
      return false;
    }
    return false;
  }
  return false;
}

std::optional<std::string> convert_call_to_jmp(std::string_view trimmed_call) {
  std::string_view rest;
  if (starts_with(trimmed_call, "callq ")) {
    rest = trimmed_call.substr(6);
  } else if (starts_with(trimmed_call, "call ")) {
    rest = trimmed_call.substr(5);
  } else {
    return std::nullopt;
  }
  if (!rest.empty() && rest.front() == '*') {
    return std::string("jmp ") + std::string(rest);
  }
  if (starts_with(rest, "__x86_indirect_thunk_")) {
    return std::nullopt;
  }
  return std::string("jmp ") + std::string(rest);
}

}  // namespace

bool optimize_tail_calls(LineStore* store, LineInfo* infos) {
  const std::size_t len = store->len();
  bool changed = false;
  bool func_suppress_tailcall = false;
  bool in_function = false;

  for (std::size_t i = 0; i < len; ++i) {
    if (infos[i].kind == LineKind::Nop) {
      continue;
    }

    if (infos[i].kind == LineKind::Label) {
      const auto trimmed = trimmed_line(store, infos[i], i);
      if (!starts_with(trimmed, ".L")) {
        func_suppress_tailcall = false;
        in_function = true;
      }
      continue;
    }
    if (infos[i].kind == LineKind::Directive) {
      const auto trimmed = trimmed_line(store, infos[i], i);
      if (trimmed == ".cfi_startproc") {
        func_suppress_tailcall = false;
        in_function = true;
      }
      continue;
    }

    if (in_function && !func_suppress_tailcall && infos[i].kind == LineKind::Other) {
      const auto trimmed = trimmed_line(store, infos[i], i);
      if ((starts_with(trimmed, "leaq ") || starts_with(trimmed, "leal ") || starts_with(trimmed, "lea ")) &&
          (trimmed.find("(%rbp)") != std::string_view::npos || trimmed.find("(%rsp)") != std::string_view::npos)) {
        func_suppress_tailcall = true;
      }
      if (starts_with(trimmed, "subq %") && ends_with(trimmed, ", %rsp")) {
        func_suppress_tailcall = true;
      }
    }

    if (infos[i].kind != LineKind::Call || func_suppress_tailcall) {
      continue;
    }

    std::size_t ret_idx = 0;
    if (!is_tail_call_candidate(store, infos, i, len, &ret_idx)) {
      continue;
    }

    const auto call_line = trimmed_line(store, infos[i], i);
    const auto jmp_text = convert_call_to_jmp(call_line);
    if (!jmp_text.has_value()) {
      continue;
    }

    mark_nop(infos[i]);
    replace_line(store, infos[ret_idx], ret_idx, "    " + *jmp_text);
    changed = true;
  }

  return changed;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
