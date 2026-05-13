#include "api.hpp"

namespace c4c::backend::aarch64::api {

c4c::backend::aarch64::module::BuildResult build_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& module) {
  return c4c::backend::aarch64::module::build(module);
}

}  // namespace c4c::backend::aarch64::api
