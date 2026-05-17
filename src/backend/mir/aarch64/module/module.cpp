#include "module.hpp"

#include "../codegen/module_compile.hpp"

namespace c4c::backend::aarch64::module {

BuildResult build(const prepare::PreparedBirModule& prepared) {
  return codegen::build_module(prepared);
}

}  // namespace c4c::backend::aarch64::module
