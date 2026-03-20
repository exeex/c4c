#include "llvm_codegen.hpp"
#include "hir_emitter.hpp"
#include "hir_to_lir.hpp"
#include "lir_printer.hpp"

#include <iostream>
#include <sstream>

namespace c4c::codegen::llvm_backend {

static std::string emit_legacy(const Module& mod) {
  HirEmitter emitter(mod);
  return emitter.emit();
}

static std::string emit_lir(const Module& mod) {
  auto lir_mod = lir::lower(mod);
  return lir::print_llvm(lir_mod);
}

std::string emit_module_native(const Module& mod, CodegenPath path) {
  switch (path) {
  case CodegenPath::Legacy:
    return emit_legacy(mod);

  case CodegenPath::Lir:
    return emit_lir(mod);

  case CodegenPath::Compare: {
    std::string legacy_ir = emit_legacy(mod);
    std::string lir_ir    = emit_lir(mod);
    if (legacy_ir == lir_ir) {
      std::cerr << "codegen compare: outputs match\n";
    } else {
      std::cerr << "codegen compare: outputs DIFFER\n";
      // Show first difference context
      std::istringstream ls(legacy_ir), rs(lir_ir);
      std::string ll, rl;
      int lineno = 0;
      int diffs  = 0;
      bool have_l, have_r;
      while ((have_l = !!std::getline(ls, ll)),
             (have_r = !!std::getline(rs, rl)),
             have_l || have_r) {
        ++lineno;
        if (!have_l) ll.clear();
        if (!have_r) rl.clear();
        if (ll != rl) {
          if (diffs < 10) {
            std::cerr << "  line " << lineno << ":\n"
                      << "    legacy: " << ll << "\n"
                      << "    lir:    " << rl << "\n";
          }
          ++diffs;
        }
      }
      if (diffs > 10) {
        std::cerr << "  ... and " << (diffs - 10)
                  << " more differing lines\n";
      }
      std::cerr << "  total differing lines: " << diffs << "\n";
    }
    // Always return legacy output so tests still pass.
    return legacy_ir;
  }
  }
  return emit_legacy(mod);  // fallback
}

}  // namespace c4c::codegen::llvm_backend
