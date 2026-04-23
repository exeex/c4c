#include "module/module_emit.hpp"

namespace c4c::backend::x86 {

// Legacy prepared-module symbol shim: keep the historic top-level entrypoint
// explicit while `module/module_emit.cpp` remains the real owner.
std::string emit_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& module) {
  return c4c::backend::x86::module::emit_prepared_module_text(module);
}

}  // namespace c4c::backend::x86
