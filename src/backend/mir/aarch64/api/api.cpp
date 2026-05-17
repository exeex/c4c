#include "api.hpp"

namespace c4c::backend::aarch64::api {

namespace prepare = c4c::backend::prepare;

c4c::backend::aarch64::codegen::CompileResult build_prepared_module(
    const prepare::PreparedBirModule& module) {
  return c4c::backend::aarch64::codegen::compile_prepared_module(module);
}

}  // namespace c4c::backend::aarch64::api
