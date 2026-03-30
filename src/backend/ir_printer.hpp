#pragma once

#include "ir.hpp"

#include <string>

namespace c4c::backend {

std::string print_backend_ir(const BackendModule& module);

}  // namespace c4c::backend
