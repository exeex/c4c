#include "x86_codegen.hpp"
#include "module/module_emit.hpp"

namespace c4c::backend::x86 {

std::string emit_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& module) {
  return c4c::backend::x86::module::emit_prepared_module_text(module);
}

}  // namespace c4c::backend::x86
