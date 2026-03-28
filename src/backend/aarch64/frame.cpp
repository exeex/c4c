#include "frame.hpp"

#include "memory.hpp"
#include "support.hpp"

namespace c4c::backend::aarch64 {

void render_entry_allocas(std::ostream& out,
                          const c4c::codegen::lir::LirFunction& function) {
  for (const auto& inst : function.alloca_insts) {
    if (!render_memory_instruction(out, inst)) {
      fail_unsupported("non-memory entry allocas");
    }
  }
}

void render_function_epilogue(std::ostream& out) { out << "}\n\n"; }

}  // namespace c4c::backend::aarch64
