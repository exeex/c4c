#include "api.hpp"

namespace c4c::backend::aarch64::api {

namespace prepare = c4c::backend::prepare;

c4c::backend::aarch64::module::BuildResult build_prepared_module(
    const prepare::PreparedBirModule& module) {
  return c4c::backend::aarch64::module::build(module);
}

}  // namespace c4c::backend::aarch64::api
