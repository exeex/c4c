#include "lowering.hpp"

namespace c4c::backend::x86::lowering {

std::string frame_comment(std::string_view function_name) {
  return std::string("# frame layout deferred for function ") + std::string(function_name);
}

}  // namespace c4c::backend::x86::lowering
