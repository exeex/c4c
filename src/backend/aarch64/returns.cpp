#include "returns.hpp"

namespace c4c::backend::aarch64 {

void render_return(std::ostream& out, const c4c::codegen::lir::LirRet& ret) {
  if (!ret.value_str.has_value()) {
    out << "  ret void\n";
    return;
  }
  out << "  ret " << ret.type_str << " " << *ret.value_str << "\n";
}

}  // namespace c4c::backend::aarch64
