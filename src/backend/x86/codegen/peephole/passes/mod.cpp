// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/passes/mod.rs
// Pass orchestration for the x86 peephole pipeline.

#include "../peephole.hpp"

#include <sstream>
#include <utility>

namespace c4c::backend::x86::codegen::peephole::passes {

namespace {

LineStore parse_line_store(std::string_view asm_text) {
  LineStore store;
  std::size_t start = 0;
  while (start < asm_text.size()) {
    const auto end = asm_text.find('\n', start);
    if (end == std::string_view::npos) {
      store.lines.emplace_back(asm_text.substr(start));
      break;
    }
    store.lines.emplace_back(asm_text.substr(start, end - start));
    start = end + 1;
  }
  if (!asm_text.empty() && asm_text.back() == '\n') {
    store.lines.emplace_back();
  }
  return store;
}

std::vector<LineInfo> classify_lines(const LineStore& store) {
  std::vector<LineInfo> infos;
  infos.reserve(store.len());
  for (std::size_t i = 0; i < store.len(); ++i) {
    infos.push_back(classify_line(store.get(i)));
  }
  return infos;
}

std::string render_line_store(const LineStore& store, const std::vector<LineInfo>& infos) {
  std::ostringstream out;
  bool first = true;
  for (std::size_t i = 0; i < store.len(); ++i) {
    if (infos[i].kind == LineKind::Nop) {
      continue;
    }
    if (!first) {
      out << '\n';
    }
    first = false;
    out << store.get(i);
  }
  if (!first) {
    out << '\n';
  }
  return out.str();
}

}  // namespace

std::string peephole_optimize(std::string asm_text) {
  auto store = parse_line_store(asm_text);
  auto infos = classify_lines(store);

  for (int round = 0; round < 4; ++round) {
    bool changed = false;
    changed |= combined_local_pass(&store, infos.data());
    changed |= fuse_movq_ext_truncation(&store, infos.data());
    changed |= eliminate_push_pop_pairs(&store, infos.data());
    if (!changed) {
      break;
    }
  }

  return render_line_store(store, infos);
}

}  // namespace c4c::backend::x86::codegen::peephole::passes
