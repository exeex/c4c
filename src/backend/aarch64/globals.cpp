#include "globals.hpp"

#include "../../codegen/shared/llvm_helpers.hpp"

namespace c4c::backend::aarch64 {

void render_module_globals(std::ostream& out,
                           const c4c::codegen::lir::LirModule& module) {
  for (const auto& global : module.globals) {
    out << c4c::codegen::llvm_helpers::llvm_global_sym(global.name) << " = "
        << global.linkage_vis << global.qualifier << global.llvm_type;
    if (!global.is_extern_decl) {
      out << " " << global.init_text;
    }
    if (global.align_bytes > 1) {
      out << ", align " << global.align_bytes;
    }
    out << "\n";
  }
}

}  // namespace c4c::backend::aarch64
