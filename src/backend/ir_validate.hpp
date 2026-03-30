#pragma once

#include "ir.hpp"

#include <string>

namespace c4c::backend {

bool validate_backend_ir(const BackendModule& module, std::string* error);

}  // namespace c4c::backend
