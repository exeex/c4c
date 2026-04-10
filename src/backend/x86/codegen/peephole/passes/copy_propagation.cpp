// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/copy_propagation.rs
// Register copy propagation pass.

#include "../peephole.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace c4c::backend::x86::codegen::peephole::passes {

namespace {

constexpr const char* kReg64Names[16] = {
    "%rax", "%rcx", "%rdx", "%rbx", "%rsp", "%rbp", "%rsi", "%rdi",
    "%r8",  "%r9",  "%r10", "%r11", "%r12", "%r13", "%r14", "%r15",
};

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

bool try_propagate_into(LineStore* store, LineInfo* infos, std::size_t index,
                        RegId src_id, RegId dst_id) {
  if (infos[index].has_indirect_mem) {
    return false;
  }

  const auto trimmed = trimmed_line(store, infos[index], index);
  if ((infos[index].reg_refs & static_cast<std::uint16_t>(1u << dst_id)) == 0) {
    return false;
  }
  if (has_implicit_reg_usage(trimmed)) {
    return false;
  }
  if (dst_id == 1 && is_shift_or_rotate(trimmed)) {
    return false;
  }

  const auto next_dest = get_dest_reg(infos[index]);
  if (next_dest == src_id) {
    return false;
  }

  std::string new_content;
  if (next_dest != dst_id) {
    new_content = replace_reg_family(trimmed, dst_id, src_id);
  } else {
    if (trimmed.find(',') == std::string_view::npos) {
      return false;
    }
    new_content = replace_reg_family_in_source(trimmed, dst_id, src_id);
  }

  if (new_content == trimmed) {
    return false;
  }

  replace_line(store, infos[index], index, "    " + new_content);
  return true;
}

}  // namespace

bool propagate_register_copies(LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();
  RegId copy_src[16];
  for (auto& reg : copy_src) {
    reg = REG_NONE;
  }

  std::size_t i = 0;
  while (i < len) {
    if (infos[i].is_barrier()) {
      for (auto& reg : copy_src) {
        reg = REG_NONE;
      }
      ++i;
      continue;
    }

    if (infos[i].is_nop()) {
      ++i;
      continue;
    }

    if (infos[i].has_indirect_mem) {
      for (auto& reg : copy_src) {
        reg = REG_NONE;
      }
      ++i;
      continue;
    }

    const auto trimmed = trimmed_line(store, infos[i], i);
    const auto copy = parse_reg_to_reg_movq(infos[i], trimmed);
    if (copy.has_value()) {
      const RegId src_id = copy->first;
      const RegId dst_id = copy->second;
      const RegId ultimate_src =
          copy_src[src_id] != REG_NONE ? copy_src[src_id] : src_id;

      if (ultimate_src != src_id && ultimate_src != dst_id) {
        replace_line(store, infos[i], i,
                     std::string("    movq ") + kReg64Names[ultimate_src] +
                         ", " + kReg64Names[dst_id]);
        changed = true;
      } else if (ultimate_src == dst_id) {
        mark_nop(infos[i]);
        changed = true;
        ++i;
        continue;
      }

      for (RegId reg = 0; reg < 16; ++reg) {
        if (copy_src[reg] == dst_id) {
          copy_src[reg] = REG_NONE;
        }
      }
      copy_src[dst_id] = ultimate_src;
      ++i;
      continue;
    }

    for (RegId reg = 0; reg < 16; ++reg) {
      const RegId src = copy_src[reg];
      if (src == REG_NONE) {
        continue;
      }
      if ((infos[i].reg_refs & static_cast<std::uint16_t>(1u << reg)) == 0) {
        continue;
      }
      if (has_implicit_reg_usage(trimmed_line(store, infos[i], i))) {
        break;
      }
      if (try_propagate_into(store, infos, i, src, reg)) {
        changed = true;
        break;
      }
    }

    const RegId dest_reg = get_dest_reg(infos[i]);
    if (dest_reg != REG_NONE && dest_reg <= REG_GP_MAX) {
      copy_src[dest_reg] = REG_NONE;
      for (RegId reg = 0; reg < 16; ++reg) {
        if (copy_src[reg] == dest_reg) {
          copy_src[reg] = REG_NONE;
        }
      }
    }

    if (has_implicit_reg_usage(trimmed_line(store, infos[i], i))) {
      for (auto& reg : copy_src) {
        reg = REG_NONE;
      }
    }

    ++i;
  }

  return changed;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
