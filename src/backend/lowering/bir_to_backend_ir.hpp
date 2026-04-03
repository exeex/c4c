#pragma once

#include "../bir.hpp"
#include "../ir.hpp"

namespace c4c::backend {

BackendModule lower_to_backend_ir(const bir::Module& module);

}  // namespace c4c::backend
