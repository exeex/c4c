// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/compare_branch.rs
// Compare-and-branch fusion pass.

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace c4c::backend::x86::codegen::peephole::passes {

struct LineInfo;
struct LineStore;

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

void mark_nop(LineInfo& info);
void replace_line(LineStore* store, LineInfo& info, std::size_t index, const std::string& text);
std::optional<std::string> parse_setcc(std::string_view line);
std::string invert_cc(std::string_view cc);

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
    const auto cc = parse_setcc(set_line);
    if (!cc.has_value()) {
      continue;
    }

    std::size_t test_idx = 0;
    bool found_test = false;
    for (std::size_t scan = 2; scan < seq_count; ++scan) {
      const auto si = seq_indices[scan];
      const auto line = trimmed_line(store, infos[si], si);
      if (line.starts_with("movzbq %al,") || line.starts_with("movzbl %al,")) {
        continue;
      }
      if (line == "cltq" || line.starts_with("movslq ")) {
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

    const auto jmp_line = trimmed_line(store, infos[seq_indices[test_idx + 1]], seq_indices[test_idx + 1]);
    bool is_jne = false;
    std::string_view branch_target;
    if (jmp_line.starts_with("jne ")) {
      is_jne = true;
      branch_target = jmp_line.substr(4);
    } else if (jmp_line.starts_with("je ")) {
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
