#include "lowering.hpp"

namespace c4c::backend::x86::lowering {

std::string_view scalar_summary() {
  return "scalar lowering owns integer arithmetic and materialization once the behavior packets land";
}

}  // namespace c4c::backend::x86::lowering
