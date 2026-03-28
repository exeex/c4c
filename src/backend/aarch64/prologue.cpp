#include "prologue.hpp"

#include "globals.hpp"
#include "support.hpp"

namespace c4c::backend::aarch64 {

void validate_module(const c4c::codegen::lir::LirModule& module) {
  if (!module.string_pool.empty()) fail_unsupported("string constants");
  if (!module.extern_decls.empty()) fail_unsupported("extern declarations");
  if (module.need_va_start || module.need_va_end || module.need_va_copy ||
      module.need_memcpy || module.need_stacksave || module.need_stackrestore ||
      module.need_abs) {
    fail_unsupported("intrinsic declarations");
  }
}

void validate_function(const c4c::codegen::lir::LirFunction& function) {
  if (!function.stack_objects.empty()) fail_unsupported("stack objects");
}

void render_module_header(std::ostream& out,
                          const c4c::codegen::lir::LirModule& module) {
  if (!module.data_layout.empty()) {
    out << "target datalayout = \"" << module.data_layout << "\"\n";
  }
  if (!module.target_triple.empty()) {
    out << "target triple = \"" << module.target_triple << "\"\n";
  }
  if (!module.data_layout.empty() || !module.target_triple.empty()) {
    out << "\n";
  }

  for (const auto& type_decl : module.type_decls) {
    out << type_decl << "\n";
  }
  render_module_globals(out, module);
  if (!module.type_decls.empty() || !module.globals.empty()) {
    out << "\n";
  }
}

void render_function_prologue(std::ostream& out,
                              const c4c::codegen::lir::LirFunction& function) {
  out << function.signature_text;
  if (!function.is_declaration) {
    out << "{\n";
  }
}

}  // namespace c4c::backend::aarch64
