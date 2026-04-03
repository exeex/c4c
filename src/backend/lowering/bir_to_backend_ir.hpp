#pragma once

#include "../bir.hpp"
#include "../ir.hpp"

namespace c4c::backend {

BackendModule lower_bir_to_backend_module(const bir::Module& module);
inline BackendModule lower_to_backend_ir(const bir::Module& module) {
  return lower_bir_to_backend_module(module);
}

}  // namespace c4c::backend
