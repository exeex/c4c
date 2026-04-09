// Mechanical C++ translation scaffold of ref/claudes-c-compiler/src/ir/lowering/types.rs
// and the adjacent types_ctype.rs / expr_types.rs helpers.
//
// This file is intentionally backend-owned and type-focused: it collects the
// legalization helpers that decide how LIR-level type information collapses to
// BIR's smaller type set.  The implementation is intentionally narrow and is
// wired into the build as a scaffold, but not yet used as the owning
// correctness path.

#include "passes.hpp"

#include "../../../codegen/lir/call_args.hpp"
#include "../../../codegen/lir/ir.hpp"

#include <charconv>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::lir_to_bir {

namespace {

std::optional<std::int64_t> parse_i64(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

std::optional<bir::TypeKind> lower_scalar_type_text(std::string_view text) {
  // The backend currently preserves a tiny legalized type surface.
  if (text == "void") {
    return bir::TypeKind::Void;
  }
  if (text == "i8") {
    return bir::TypeKind::I8;
  }
  if (text == "i32") {
    return bir::TypeKind::I32;
  }
  if (text == "i64") {
    return bir::TypeKind::I64;
  }
  return std::nullopt;
}

std::optional<bir::TypeKind> lower_function_signature_return_type(std::string_view signature_text) {
  const auto line = c4c::codegen::lir::trim_lir_arg_text(signature_text);
  const auto first_space = line.find(' ');
  const auto at_pos = line.find('@');
  if (first_space == std::string_view::npos || at_pos == std::string_view::npos ||
      first_space >= at_pos) {
    return std::nullopt;
  }

  return lower_scalar_type_text(
      c4c::codegen::lir::trim_lir_arg_text(line.substr(first_space + 1, at_pos - first_space - 1)));
}

unsigned scalar_type_bit_width(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::Void:
      return 0;
    case bir::TypeKind::I8:
      return 8;
    case bir::TypeKind::I32:
      return 32;
    case bir::TypeKind::I64:
      return 64;
    case bir::TypeKind::I128:
      return 128;
    case bir::TypeKind::Ptr:
      return static_cast<unsigned>(sizeof(void*) * 8);
    case bir::TypeKind::F32:
      return 32;
    case bir::TypeKind::F64:
      return 64;
    case bir::TypeKind::F128:
      return 128;
  }
  return 0;
}

std::optional<bir::TypeKind> lower_scalar_type(const c4c::codegen::lir::LirTypeRef& type) {
  if (type.kind() != c4c::codegen::lir::LirTypeKind::Integer) {
    return std::nullopt;
  }

  const auto integer_width = type.integer_bit_width();
  if (!integer_width.has_value()) {
    return std::nullopt;
  }

  switch (*integer_width) {
    case 8:
      return bir::TypeKind::I8;
    case 32:
      return bir::TypeKind::I32;
    case 64:
      return bir::TypeKind::I64;
    default:
      return std::nullopt;
  }
}

bool lir_type_has_integer_width(const c4c::codegen::lir::LirTypeRef& type,
                                unsigned bit_width) {
  return type.kind() == c4c::codegen::lir::LirTypeKind::Integer &&
         type.integer_bit_width() == bit_width;
}

bool lir_type_matches_integer_width(const c4c::codegen::lir::LirTypeRef& type,
                                    unsigned bit_width) {
  if (lir_type_has_integer_width(type, bit_width)) {
    return true;
  }

  const auto lowered_type = lower_scalar_type_text(type.str());
  return lowered_type.has_value() && scalar_type_bit_width(*lowered_type) == bit_width;
}

bool lir_type_is_pointer_like(const c4c::codegen::lir::LirTypeRef& type) {
  if (type.kind() == c4c::codegen::lir::LirTypeKind::Pointer) {
    return true;
  }

  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(type.str());
  return trimmed == "ptr" || (!trimmed.empty() && trimmed.back() == '*');
}

std::optional<bir::TypeKind> lower_minimal_scalar_type(const c4c::TypeSpec& type) {
  if (type.ptr_level != 0 || type.array_rank != 0) {
    return std::nullopt;
  }
  if (type.base == TB_CHAR || type.base == TB_SCHAR || type.base == TB_UCHAR) {
    return bir::TypeKind::I8;
  }
  if (type.base == TB_INT) {
    return bir::TypeKind::I32;
  }
  if (type.base == TB_LONG || type.base == TB_ULONG || type.base == TB_LONGLONG ||
      type.base == TB_ULONGLONG) {
    return bir::TypeKind::I64;
  }
  return std::nullopt;
}

std::optional<bir::TypeKind> lower_minimal_call_arg_type_text(std::string_view text) {
  if (const auto scalar = lower_scalar_type_text(text); scalar.has_value()) {
    return scalar;
  }

  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed == "ptr" || (!trimmed.empty() && trimmed.back() == '*')) {
    return bir::TypeKind::Ptr;
  }
  return std::nullopt;
}

bool immediate_fits_type(std::int64_t value, bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::Void:
      return false;
    case bir::TypeKind::I8:
      return value >= -128 && value <= 127;
    case bir::TypeKind::I32:
      return value >= std::numeric_limits<std::int32_t>::min() &&
             value <= std::numeric_limits<std::int32_t>::max();
    case bir::TypeKind::I64:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return true;
  }
  return false;
}

}  // namespace

// --- Translation scaffold --------------------------------------------------
//
// The functions below are the backend-facing legalization entry points that
// mirror the reference Rust lowerer.  They are kept as small, local helpers so
// the eventual split out of type logic from lir_to_bir.cpp has a clear home.

std::optional<bir::TypeKind> legalize_function_decl_return_type(
    const c4c::codegen::lir::LirFunction& function) {
  if (const auto lowered_type = lower_minimal_scalar_type(function.return_type);
      lowered_type.has_value()) {
    return lowered_type;
  }
  return lower_function_signature_return_type(function.signature_text);
}

std::optional<bir::TypeKind> legalize_extern_decl_return_type(
    const c4c::codegen::lir::LirExternDecl& decl) {
  if (const auto lowered_type = lower_scalar_type(decl.return_type);
      lowered_type.has_value()) {
    return lowered_type;
  }
  return lower_scalar_type_text(c4c::codegen::lir::trim_lir_arg_text(decl.return_type_str));
}

std::optional<bir::TypeKind> legalize_function_return_type(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::codegen::lir::LirRet& ret) {
  if (const auto lowered_type = lower_minimal_scalar_type(function.return_type);
      lowered_type.has_value()) {
    return lowered_type;
  }
  if (const auto lowered_type = lower_function_signature_return_type(function.signature_text);
      lowered_type.has_value()) {
    return lowered_type;
  }
  if (const auto lowered_type = lower_scalar_type(ret.type_str); lowered_type.has_value()) {
    return lowered_type;
  }
  return std::nullopt;
}

std::optional<bir::TypeKind> legalize_global_type(
    const c4c::codegen::lir::LirGlobal& global) {
  if (const auto lowered_type = lower_minimal_scalar_type(global.type);
      lowered_type.has_value()) {
    return lowered_type;
  }
  return lower_scalar_type_text(global.llvm_type);
}

std::optional<bir::TypeKind> legalize_call_arg_type(std::string_view text) {
  return lower_minimal_call_arg_type_text(text);
}

bool legalize_lir_type_is_pointer_like(const c4c::codegen::lir::LirTypeRef& type) {
  return lir_type_is_pointer_like(type);
}

bool legalize_lir_type_is_i32(const c4c::codegen::lir::LirTypeRef& type) {
  return lir_type_matches_integer_width(type, 32);
}

std::size_t legalize_type_size_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::Void:
      return 0;
    case bir::TypeKind::I8:
      return 1;
    case bir::TypeKind::I32:
      return 4;
    case bir::TypeKind::I64:
      return 8;
    case bir::TypeKind::I128:
      return 16;
    case bir::TypeKind::Ptr:
      return sizeof(void*);
    case bir::TypeKind::F32:
      return 4;
    case bir::TypeKind::F64:
      return 8;
    case bir::TypeKind::F128:
      return 16;
  }
  return 0;
}

std::size_t legalize_type_align_bytes(bir::TypeKind type) {
  // Placeholder alignment model: keep scalar legalization aligned with size.
  // This mirrors the reference lowerer's preference for size-aware alignment
  // decisions without trying to recreate the full C ABI matrix yet.
  const auto size = legalize_type_size_bytes(type);
  return size == 0 ? 1 : size;
}

bool legalize_types_compatible(bir::TypeKind lhs, bir::TypeKind rhs) {
  // BIR currently carries a reduced scalar set, so compatibility is a simple
  // equality check once the values have been legalized.
  return lhs == rhs;
}

bool legalize_immediate_fits_type(std::int64_t value, bir::TypeKind type) {
  return immediate_fits_type(value, type);
}

std::optional<bir::Value> legalize_immediate_or_name(std::string_view value_text,
                                                     bir::TypeKind type) {
  if (value_text.empty()) {
    return std::nullopt;
  }
  if (value_text.front() == '%') {
    return bir::Value::named(type, std::string(value_text));
  }

  const auto immediate = parse_i64(value_text);
  if (!immediate.has_value() || !immediate_fits_type(*immediate, type)) {
    return std::nullopt;
  }

  switch (type) {
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(static_cast<std::int8_t>(*immediate));
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(static_cast<std::int32_t>(*immediate));
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(*immediate);
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

}  // namespace c4c::backend::lir_to_bir

void c4c::backend::lir_to_bir::record_type_legalization_scaffold_notes(
    const c4c::codegen::lir::LirModule& module,
    std::vector<BirLoweringNote>* notes) {
  (void)module;
  if (notes == nullptr) {
    return;
  }
  notes->push_back(BirLoweringNote{
      .phase = "type-legalization",
      .message = "using split type legalization helpers from lir_to_bir/types.cpp scaffold",
  });
}
