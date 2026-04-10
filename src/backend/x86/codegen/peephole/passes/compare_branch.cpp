// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/compare_branch.rs
// Compare-and-branch fusion pass.

#include "../peephole.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::x86::codegen::peephole::passes {

namespace {

void mark_nop(LineInfo& info) { info.kind = LineKind::Nop; }

void replace_line(LineStore* store, LineInfo& info, std::size_t index,
                  const std::string& text) {
  store->get(index) = text;
  info.trim_start = 0;
}

std::optional<std::string> parse_setcc_text(std::string_view line) {
  if (!starts_with(line, "set")) {
    return std::nullopt;
  }
  const auto space = line.find(' ');
  if (space == std::string_view::npos || space <= 3) {
    return std::nullopt;
  }
  return std::string(line.substr(3, space - 3));
}

std::string invert_cc(std::string_view cc) {
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

static constexpr std::size_t MAX_TRACKED_STORE_LOAD_OFFSETS = 4;
static constexpr std::size_t CMP_FUSION_LOOKAHEAD = 8;

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

static std::string trimmed_line(const LineStore* store, const LineInfo& info, std::size_t index) {
  return std::string(store->get(index).substr(info.trim_start));
}

bool has_unmatched_tracked_store(const LineStore* store, const LineInfo* infos,
                                 std::size_t range_start, std::size_t range_end,
                                 const std::int32_t* store_offsets,
                                 std::size_t store_count) {
  std::int32_t load_offsets[MAX_TRACKED_STORE_LOAD_OFFSETS] = {};
  std::size_t load_count = 0;

  for (std::size_t i = range_start; i <= range_end; ++i) {
    std::optional<std::int32_t> offset;
    if (infos[i].kind == LineKind::LoadRbp) {
      offset = infos[i].rbp_offset;
    } else if (infos[i].kind == LineKind::Nop) {
      const auto original = classify_line(store->get(i));
      if (original.kind == LineKind::LoadRbp) {
        offset = original.rbp_offset;
      }
    }

    if (offset.has_value() && load_count < MAX_TRACKED_STORE_LOAD_OFFSETS) {
      load_offsets[load_count++] = *offset;
    }
  }

  for (std::size_t si = 0; si < store_count; ++si) {
    bool matched = false;
    for (std::size_t li = 0; li < load_count; ++li) {
      if (load_offsets[li] == store_offsets[si]) {
        matched = true;
        break;
      }
    }
    if (!matched) {
      return true;
    }
  }

  return false;
}

}  // namespace

bool fuse_compare_and_branch(LineStore* store, LineInfo* infos) {
  bool changed = false;
  const std::size_t len = store->len();

  for (std::size_t i = 0; i < len; ++i) {
    if (infos[i].kind != LineKind::Cmp) {
      continue;
    }

    std::array<std::size_t, CMP_FUSION_LOOKAHEAD> seq_indices{};
    seq_indices[0] = i;
    std::size_t rest[CMP_FUSION_LOOKAHEAD - 1] = {};
    const auto rest_count = collect_non_nop_indices(infos, i + 1, len, rest, CMP_FUSION_LOOKAHEAD - 1);
    for (std::size_t k = 0; k < rest_count; ++k) {
      seq_indices[k + 1] = rest[k];
    }
    const std::size_t seq_count = 1 + rest_count;
    if (seq_count < 4) {
      continue;
    }

    if (infos[seq_indices[1]].kind != LineKind::SetCC) {
      continue;
    }

    const auto set_line = trimmed_line(store, infos[seq_indices[1]], seq_indices[1]);
    const auto cc = parse_setcc_text(set_line);
    if (!cc.has_value()) {
      continue;
    }

    std::int32_t tracked_store_offsets[MAX_TRACKED_STORE_LOAD_OFFSETS] = {};
    std::size_t tracked_store_count = 0;
    std::size_t test_idx = 0;
    bool found_test = false;
    for (std::size_t scan = 2; scan < seq_count; ++scan) {
      const auto si = seq_indices[scan];
      const auto line = trimmed_line(store, infos[si], si);
      if (starts_with(line, "movzbq %al,") || starts_with(line, "movzbl %al,")) {
        continue;
      }
      if (infos[si].kind == LineKind::StoreRbp) {
        if (tracked_store_count == MAX_TRACKED_STORE_LOAD_OFFSETS) {
          tracked_store_count = MAX_TRACKED_STORE_LOAD_OFFSETS + 1;
          break;
        }
        tracked_store_offsets[tracked_store_count++] = infos[si].rbp_offset;
        continue;
      }
      if (infos[si].kind == LineKind::LoadRbp) {
        continue;
      }
      if (line == "cltq" || starts_with(line, "movslq ")) {
        continue;
      }
      if (line == "testq %rax, %rax" || line == "testl %eax, %eax") {
        test_idx = scan;
        found_test = true;
        break;
      }
      break;
    }
    if (!found_test || test_idx + 1 >= seq_count) {
      continue;
    }

    if (tracked_store_count > MAX_TRACKED_STORE_LOAD_OFFSETS) {
      continue;
    }
    if (tracked_store_count > 0 &&
        has_unmatched_tracked_store(store, infos, seq_indices[1], seq_indices[test_idx],
                                    tracked_store_offsets, tracked_store_count)) {
      continue;
    }

    const auto jmp_line = trimmed_line(store, infos[seq_indices[test_idx + 1]], seq_indices[test_idx + 1]);
    bool is_jne = false;
    std::string_view branch_target;
    if (starts_with(jmp_line, "jne ")) {
      is_jne = true;
      branch_target = jmp_line.substr(4);
    } else if (starts_with(jmp_line, "je ")) {
      is_jne = false;
      branch_target = jmp_line.substr(3);
    } else {
      continue;
    }

    const auto fused_cc = is_jne ? *cc : invert_cc(*cc);
    const auto fused_jcc = std::string("    j") + fused_cc + " " + std::string(branch_target);

    for (std::size_t s = 1; s <= test_idx; ++s) {
      mark_nop(infos[seq_indices[s]]);
    }
    replace_line(store, infos[seq_indices[test_idx + 1]], seq_indices[test_idx + 1], fused_jcc);
    changed = true;
  }

  return changed;
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
