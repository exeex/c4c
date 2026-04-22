#include "module_emit.hpp"

namespace c4c::backend::x86::module {

std::string emit_prepared_module_text(
    const c4c::backend::prepare::PreparedBirModule& module) {
  return c4c::backend::x86::emit_prepared_module(module);
}

}  // namespace c4c::backend::x86::module
