#pragma once

#include "../../../shared/struct_name_table.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace c4c::backend::bir {

enum class TypeKind : std::uint8_t {
  Void,
  I1,
  I8,
  I16,
  I32,
  I64,
  F32,
  F64,
  Pointer,
  Integer,
  Floating,
  Vector,
  VrmRegister,
  Array,
  Struct,
  Function,
  Opaque,
};

enum class StructuredTypeBase : std::uint8_t { Void };

struct StructuredTypeSpecFacts {
  StructuredTypeBase base = StructuredTypeBase::Void;
  int pointer_level = 0;
  bool is_lvalue_reference = false;
  bool is_rvalue_reference = false;
  int array_rank = 0;
  bool is_pointer_to_array = false;
  int inner_array_rank = 0;
  bool is_function_pointer = false;
};

struct ArrayTypeFacts {
  TypeKind element_kind = TypeKind::Void;
  std::uint32_t element_bit_width = 0;
  int element_pointer_depth = 0;
  std::vector<std::int64_t> dimensions;
};

struct PointerTypeFacts {
  TypeKind pointee_kind = TypeKind::Void;
  std::uint32_t pointee_bit_width = 0;
  int pointer_depth = 0;
};

inline bool operator==(const PointerTypeFacts& lhs,
                       const PointerTypeFacts& rhs) noexcept {
  return lhs.pointee_kind == rhs.pointee_kind &&
         lhs.pointee_bit_width == rhs.pointee_bit_width &&
         lhs.pointer_depth == rhs.pointer_depth;
}

inline bool operator!=(const PointerTypeFacts& lhs,
                       const PointerTypeFacts& rhs) noexcept {
  return !(lhs == rhs);
}

inline bool operator==(const ArrayTypeFacts& lhs,
                       const ArrayTypeFacts& rhs) noexcept {
  return lhs.element_kind == rhs.element_kind &&
         lhs.element_bit_width == rhs.element_bit_width &&
         lhs.element_pointer_depth == rhs.element_pointer_depth &&
         lhs.dimensions == rhs.dimensions;
}

inline bool operator!=(const ArrayTypeFacts& lhs,
                       const ArrayTypeFacts& rhs) noexcept {
  return !(lhs == rhs);
}

inline bool operator==(const StructuredTypeSpecFacts& lhs,
                       const StructuredTypeSpecFacts& rhs) noexcept {
  return lhs.base == rhs.base && lhs.pointer_level == rhs.pointer_level &&
         lhs.is_lvalue_reference == rhs.is_lvalue_reference &&
         lhs.is_rvalue_reference == rhs.is_rvalue_reference &&
         lhs.array_rank == rhs.array_rank &&
         lhs.is_pointer_to_array == rhs.is_pointer_to_array &&
         lhs.inner_array_rank == rhs.inner_array_rank &&
         lhs.is_function_pointer == rhs.is_function_pointer;
}

inline bool operator!=(const StructuredTypeSpecFacts& lhs,
                       const StructuredTypeSpecFacts& rhs) noexcept {
  return !(lhs == rhs);
}

struct Type {
  TypeKind kind = TypeKind::Void;
  std::uint32_t bit_width = 0;
  c4c::StructNameId struct_name_id = c4c::kInvalidStructName;
  std::string spelling;
  std::optional<StructuredTypeSpecFacts> structured_spec;
  std::optional<ArrayTypeFacts> array_facts;
  std::optional<PointerTypeFacts> pointer_facts;

  Type() = default;
  Type(TypeKind type_kind) : kind(type_kind) {
    switch (kind) {
      case TypeKind::I1: bit_width = 1; break;
      case TypeKind::I8: bit_width = 8; break;
      case TypeKind::I16: bit_width = 16; break;
      case TypeKind::I32:
      case TypeKind::F32: bit_width = 32; break;
      case TypeKind::I64:
      case TypeKind::F64: bit_width = 64; break;
      case TypeKind::Pointer: spelling = "ptr"; break;
      default: break;
    }
  }

  Type(TypeKind type_kind, std::uint32_t width, std::string rendered = {},
       c4c::StructNameId name_id = c4c::kInvalidStructName)
      : kind(type_kind),
        bit_width(width),
        struct_name_id(name_id),
        spelling(std::move(rendered)) {}
};

inline bool operator==(const Type& lhs, const Type& rhs) noexcept {
  if (lhs.array_facts != rhs.array_facts) return false;
  if (lhs.pointer_facts != rhs.pointer_facts) return false;
  const auto integer_width = [](const Type& type) -> std::uint32_t {
    switch (type.kind) {
      case TypeKind::I1: return 1;
      case TypeKind::I8: return 8;
      case TypeKind::I16: return 16;
      case TypeKind::I32: return 32;
      case TypeKind::I64: return 64;
      case TypeKind::Integer: return type.bit_width;
      default: return 0;
    }
  };
  const auto floating_width = [](const Type& type) -> std::uint32_t {
    switch (type.kind) {
      case TypeKind::F32: return 32;
      case TypeKind::F64: return 64;
      case TypeKind::Floating: return type.bit_width;
      default: return 0;
    }
  };
  const std::uint32_t lhs_integer = integer_width(lhs);
  const std::uint32_t rhs_integer = integer_width(rhs);
  if (lhs_integer || rhs_integer)
    return lhs_integer != 0 && lhs_integer == rhs_integer;
  const std::uint32_t lhs_float = floating_width(lhs);
  const std::uint32_t rhs_float = floating_width(rhs);
  if (lhs_float || rhs_float)
    return lhs_float != 0 && lhs_float == rhs_float &&
           (lhs.spelling.empty() || rhs.spelling.empty() ||
            lhs.spelling == rhs.spelling);
  return lhs.kind == rhs.kind && lhs.bit_width == rhs.bit_width &&
         lhs.struct_name_id == rhs.struct_name_id &&
         lhs.spelling == rhs.spelling &&
         lhs.structured_spec == rhs.structured_spec;
}
inline bool operator!=(const Type& lhs, const Type& rhs) noexcept {
  return !(lhs == rhs);
}

inline bool is_well_formed(const Type& type) {
  if (type.kind != TypeKind::Array && type.array_facts) return false;
  if (type.kind != TypeKind::Pointer && type.pointer_facts) return false;
  if (type.structured_spec) {
    const auto& spec = *type.structured_spec;
    if (type.kind != TypeKind::Void ||
        spec.base != StructuredTypeBase::Void || spec.pointer_level != 0 ||
        spec.is_lvalue_reference || spec.is_rvalue_reference ||
        spec.array_rank != 0 || spec.is_pointer_to_array ||
        spec.inner_array_rank != 0 || spec.is_function_pointer)
      return false;
  }
  const bool no_name = type.struct_name_id == c4c::kInvalidStructName;
  switch (type.kind) {
    case TypeKind::Void:
      return type.bit_width == 0 && no_name &&
             (type.spelling.empty() || type.spelling == "void");
    case TypeKind::I1: return type.bit_width == 1 && no_name;
    case TypeKind::I8: return type.bit_width == 8 && no_name;
    case TypeKind::I16: return type.bit_width == 16 && no_name;
    case TypeKind::I32: return type.bit_width == 32 && no_name;
    case TypeKind::I64: return type.bit_width == 64 && no_name;
    case TypeKind::F32: return type.bit_width == 32 && no_name;
    case TypeKind::F64: return type.bit_width == 64 && no_name;
    case TypeKind::Integer:
      return type.bit_width != 0 && no_name &&
             type.spelling == "i" + std::to_string(type.bit_width);
    case TypeKind::Floating:
      return no_name &&
             ((type.bit_width == 16 && type.spelling == "half") ||
              (type.bit_width == 32 && type.spelling == "float") ||
              (type.bit_width == 64 && type.spelling == "double") ||
              (type.bit_width == 80 && type.spelling == "x86_fp80") ||
              (type.bit_width == 128 && type.spelling == "fp128"));
    case TypeKind::Pointer:
      if (type.bit_width != 0 || !no_name || type.spelling != "ptr")
        return false;
      if (!type.pointer_facts) return true;
      if (type.pointer_facts->pointer_depth <= 0) return false;
      if (type.pointer_facts->pointee_kind == TypeKind::Integer)
        return type.pointer_facts->pointee_bit_width != 0;
      if (type.pointer_facts->pointee_kind != TypeKind::Floating) return false;
      switch (type.pointer_facts->pointee_bit_width) {
        case 16:
        case 32:
        case 64:
        case 80:
        case 128: return true;
        default: return false;
      }
    case TypeKind::VrmRegister:
      return (type.bit_width == 1 || type.bit_width == 2 ||
              type.bit_width == 4 || type.bit_width == 8) &&
             no_name &&
             type.spelling == "c4c.vrm" + std::to_string(type.bit_width);
    case TypeKind::Struct:
      return type.bit_width == 0 && !type.spelling.empty() &&
             ((!no_name) ||
              (type.spelling.size() >= 2 && type.spelling.front() == '{' &&
               type.spelling.back() == '}'));
    case TypeKind::Vector:
      return type.bit_width == 0 && no_name && type.spelling.size() >= 2 &&
             type.spelling.front() == '<' && type.spelling.back() == '>';
    case TypeKind::Array: {
      if (type.bit_width != 0 || !no_name || type.spelling.size() < 2 ||
          type.spelling.front() != '[' || type.spelling.back() != ']')
        return false;
      if (!type.array_facts) return true;
      if (type.array_facts->dimensions.empty()) return false;
      for (const auto dimension : type.array_facts->dimensions)
        if (dimension <= 0) return false;
      std::string element_spelling;
      if (type.array_facts->element_kind == TypeKind::Integer) {
        if (type.array_facts->element_bit_width == 0) return false;
        element_spelling =
            "i" + std::to_string(type.array_facts->element_bit_width);
      } else if (type.array_facts->element_kind == TypeKind::Floating) {
        switch (type.array_facts->element_bit_width) {
          case 16: element_spelling = "half"; break;
          case 32: element_spelling = "float"; break;
          case 64: element_spelling = "double"; break;
          case 80: element_spelling = "x86_fp80"; break;
          case 128: element_spelling = "fp128"; break;
          default: return false;
        }
      } else {
        return false;
      }
      if (type.array_facts->element_pointer_depth > 0)
        element_spelling = "ptr";
      else if (type.array_facts->element_pointer_depth < 0)
        return false;
      for (auto dimension = type.array_facts->dimensions.rbegin();
           dimension != type.array_facts->dimensions.rend(); ++dimension)
        element_spelling =
            "[" + std::to_string(*dimension) + " x " + element_spelling + "]";
      return type.spelling == element_spelling;
    }
    case TypeKind::Function:
      return type.bit_width == 0 && no_name &&
             type.spelling.find('(') != std::string::npos &&
             type.spelling.find(')') != std::string::npos;
    case TypeKind::Opaque:
      return type.bit_width == 0 && no_name && !type.spelling.empty() &&
             type.spelling.front() == '%';
  }
  return false;
}

}  // namespace c4c::backend::bir
