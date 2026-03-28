#include "globals.hpp"

#include "../../codegen/shared/llvm_helpers.hpp"

namespace c4c::backend::aarch64 {

void render_module_string_pool(std::ostream& out,
                               const c4c::codegen::lir::LirModule& module) {
  for (const auto& string_const : module.string_pool) {
    if (string_const.byte_length < 0) {
      out << string_const.pool_name << " = private unnamed_addr constant "
          << string_const.raw_bytes << "\n";
      continue;
    }

    out << string_const.pool_name << " = private unnamed_addr constant ["
        << string_const.byte_length << " x i8] c\"" << string_const.raw_bytes
        << "\\00\"\n";
  }
}

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

void render_module_extern_decls(std::ostream& out,
                                const c4c::codegen::lir::LirModule& module) {
  for (const auto& decl : module.extern_decls) {
    out << "declare " << decl.return_type_str << " "
        << c4c::codegen::llvm_helpers::llvm_global_sym(decl.name) << "(...)\n";
  }
}

void render_module_intrinsic_decls(std::ostream& out,
                                   const c4c::codegen::lir::LirModule& module) {
  if (module.need_va_start) {
    out << "declare void @llvm.va_start.p0(ptr)\n";
  }
  if (module.need_va_end) {
    out << "declare void @llvm.va_end.p0(ptr)\n";
  }
  if (module.need_va_copy) {
    out << "declare void @llvm.va_copy.p0.p0(ptr, ptr)\n";
  }
  if (module.need_memcpy) {
    out << "declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)\n";
  }
  if (module.need_ptrmask) {
    out << "declare ptr @llvm.ptrmask.p0.i64(ptr, i64)\n";
  }
}

}  // namespace c4c::backend::aarch64
