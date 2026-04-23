#include "lowering.hpp"

namespace c4c::backend::x86::lowering {

std::optional<std::string_view> memory_size(c4c::backend::bir::TypeKind type) {
  using c4c::backend::bir::TypeKind;
  switch (type) {
    case TypeKind::I8:
      return std::string_view("BYTE PTR");
    case TypeKind::I16:
      return std::string_view("WORD PTR");
    case TypeKind::I32:
    case TypeKind::I1:
    case TypeKind::F32:
      return std::string_view("DWORD PTR");
    case TypeKind::I64:
    case TypeKind::Ptr:
    case TypeKind::F64:
      return std::string_view("QWORD PTR");
    default:
      return std::nullopt;
  }
}

std::string stack_mem(std::size_t byte_offset, std::string_view size_name) {
  return std::string(size_name) + " [rsp + " + std::to_string(byte_offset) + "]";
}

}  // namespace c4c::backend::x86::lowering
