#pragma once

#include "bir.hpp"

#include <string>

namespace c4c::backend::bir {

bool validate(const Module& module, std::string* error);

}  // namespace c4c::backend::bir
