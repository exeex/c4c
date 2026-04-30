#include "calling_convention.hpp"
#include "../shared/llvm_helpers.hpp"

#include <algorithm>

namespace c4c::codegen::llvm_backend {
namespace {

using hir::HirStructDef;
using hir::HirStructField;
using hir::Module;
using c4c::TypeSpec;

constexpr int kAmd64GpAreaBytes = 48;
constexpr int kAmd64FpAreaBytes = 176;
constexpr int kAmd64VaListStructBytes = 24;

TypeSpec drop_one_array_dim(TypeSpec ts) {
  if (ts.array_rank <= 0) return ts;
  ts.array_rank -= 1;
  if (ts.array_rank > 0) {
    for (int i = 0; i < ts.array_rank; ++i) {
      ts.array_dims[i] = ts.array_dims[i + 1];
    }
    ts.array_size = ts.array_dims[0];
  } else {
    ts.array_size = -1;
  }
  return ts;
}

int primitive_size_bytes(c4c::TypeBase base) {
  using c4c::TypeBase;
  switch (base) {
    case TypeBase::TB_BOOL:
    case TypeBase::TB_CHAR:
    case TypeBase::TB_SCHAR:
    case TypeBase::TB_UCHAR:
      return 1;
    case TypeBase::TB_SHORT:
    case TypeBase::TB_USHORT:
      return 2;
    case TypeBase::TB_INT:
    case TypeBase::TB_UINT:
    case TypeBase::TB_FLOAT:
    case TypeBase::TB_ENUM:
      return 4;
    case TypeBase::TB_LONG:
    case TypeBase::TB_ULONG:
    case TypeBase::TB_LONGLONG:
    case TypeBase::TB_ULONGLONG:
    case TypeBase::TB_DOUBLE:
      return 8;
    case TypeBase::TB_LONGDOUBLE:
      return 16;
    case TypeBase::TB_INT128:
    case TypeBase::TB_UINT128:
      return 16;
    case TypeBase::TB_COMPLEX_FLOAT:
      return 8;
    case TypeBase::TB_COMPLEX_DOUBLE:
      return 16;
    case TypeBase::TB_COMPLEX_LONGDOUBLE:
      return 32;
    default:
      return 0;
  }
}

const HirStructDef* lookup_struct(const Module& mod, const TypeSpec& ts) {
  if (!ts.tag || !ts.tag[0]) return nullptr;
  auto it = mod.struct_defs.find(ts.tag);
  if (it == mod.struct_defs.end()) return nullptr;
  return &it->second;
}

Amd64ArgClass merge_class(Amd64ArgClass a, Amd64ArgClass b) {
  if (a == b) return a;
  if (a == Amd64ArgClass::None) return b;
  if (b == Amd64ArgClass::None) return a;
  if (a == Amd64ArgClass::Memory || b == Amd64ArgClass::Memory) {
    return Amd64ArgClass::Memory;
  }
  if (a == Amd64ArgClass::Integer || b == Amd64ArgClass::Integer) {
    return Amd64ArgClass::Integer;
  }
  if (a == Amd64ArgClass::X87 || a == Amd64ArgClass::X87Up ||
      b == Amd64ArgClass::X87 || b == Amd64ArgClass::X87Up) {
    return Amd64ArgClass::Memory;
  }
  if (a == Amd64ArgClass::ComplexX87 || b == Amd64ArgClass::ComplexX87) {
    return Amd64ArgClass::Memory;
  }
  return Amd64ArgClass::SSE;
}

void set_memory(std::array<Amd64ArgClass, 2>& classes) {
  classes[0] = Amd64ArgClass::Memory;
  classes[1] = Amd64ArgClass::Memory;
}

void mark_class(std::array<Amd64ArgClass, 2>& classes,
                size_t offset, size_t size,
                Amd64ArgClass cls) {
  if (size == 0) return;
  if (offset >= 16 || offset + size > 16) {
    set_memory(classes);
    return;
  }
  size_t start = offset / 8;
  size_t end = (offset + size + 7) / 8;
  for (size_t i = start; i < end && i < classes.size(); ++i) {
    classes[i] = merge_class(classes[i], cls);
    if (classes[0] == Amd64ArgClass::Memory) return;
    if (cls == Amd64ArgClass::SSE) cls = Amd64ArgClass::SSEUp;
  }
}

int array_elem_count(const TypeSpec& ts) {
  if (ts.array_rank <= 0) return 0;
  if (ts.array_dims[0] < 0) return 0;
  return static_cast<int>(ts.array_dims[0]);
}

void classify_type(const Module& mod, const TypeSpec& ts, size_t offset,
                   std::array<Amd64ArgClass, 2>& classes);

void classify_struct_fields(const Module& mod, const HirStructDef& sd,
                            size_t base_offset,
                            std::array<Amd64ArgClass, 2>& classes) {
  if (sd.size_bytes > 16) {
    set_memory(classes);
    return;
  }
  if (sd.is_union) {
    for (const HirStructField& f : sd.fields) {
      classify_type(mod, f.elem_type, base_offset, classes);
      if (classes[0] == Amd64ArgClass::Memory) return;
    }
    return;
  }
  for (const HirStructField& f : sd.fields) {
    classify_type(mod, f.elem_type, base_offset + f.offset_bytes, classes);
    if (classes[0] == Amd64ArgClass::Memory) return;
  }
}

void classify_array(const Module& mod, const TypeSpec& ts,
                    size_t offset, std::array<Amd64ArgClass, 2>& classes) {
  if (ts.array_rank <= 0) {
    classify_type(mod, ts, offset, classes);
    return;
  }
  const int count = array_elem_count(ts);
  if (count <= 0) {
    set_memory(classes);
    return;
  }
  TypeSpec elem = drop_one_array_dim(ts);
  int elem_size = amd64_type_size_bytes(elem, mod);
  if (elem_size == 0) {
    set_memory(classes);
    return;
  }
  for (int i = 0; i < count; ++i) {
    classify_type(mod, elem, offset + static_cast<size_t>(i) * elem_size, classes);
    if (classes[0] == Amd64ArgClass::Memory) return;
  }
}

void classify_type(const Module& mod, const TypeSpec& ts, size_t offset,
                   std::array<Amd64ArgClass, 2>& classes) {
  int size = amd64_type_size_bytes(ts, mod);
  if (size <= 0) return;
  if (size > 16) {
    set_memory(classes);
    return;
  }
  if (ts.array_rank > 0) {
    classify_array(mod, ts, offset, classes);
    return;
  }
  using c4c::TypeBase;
  if (ts.ptr_level > 0 || ts.is_fn_ptr) {
    mark_class(classes, offset, size, Amd64ArgClass::Integer);
    return;
  }
  if ((ts.base == TypeBase::TB_STRUCT || ts.base == TypeBase::TB_UNION) &&
      ts.tag && ts.tag[0]) {
    if (const HirStructDef* sd = lookup_struct(mod, ts)) {
      classify_struct_fields(mod, *sd, offset, classes);
    } else {
      set_memory(classes);
    }
    return;
  }
  switch (ts.base) {
    case TypeBase::TB_FLOAT:
    case TypeBase::TB_DOUBLE:
      mark_class(classes, offset, size, Amd64ArgClass::SSE);
      return;
    case TypeBase::TB_COMPLEX_FLOAT:
      mark_class(classes, offset, size, Amd64ArgClass::SSE);
      return;
    case TypeBase::TB_COMPLEX_DOUBLE:
      mark_class(classes, offset, 8, Amd64ArgClass::SSE);
      mark_class(classes, offset + 8, size - 8, Amd64ArgClass::SSEUp);
      return;
    case TypeBase::TB_LONGDOUBLE:
    case TypeBase::TB_COMPLEX_LONGDOUBLE:
      set_memory(classes);
      return;
    default:
      break;
  }
  mark_class(classes, offset, size, Amd64ArgClass::Integer);
}

void finalize_classes(std::array<Amd64ArgClass, 2>& classes, int size) {
  if (classes[0] == Amd64ArgClass::Memory || classes[1] == Amd64ArgClass::Memory) {
    set_memory(classes);
    return;
  }
  if (classes[0] == Amd64ArgClass::None) classes[0] = Amd64ArgClass::Integer;
  if (size > 8 && classes[1] == Amd64ArgClass::None) classes[1] = classes[0];
  if (classes[0] == Amd64ArgClass::SSEUp) classes[0] = Amd64ArgClass::SSE;
  if (classes[1] == Amd64ArgClass::SSEUp && classes[0] != Amd64ArgClass::SSE) {
    classes[1] = Amd64ArgClass::SSE;
  }
  if (classes[0] == Amd64ArgClass::X87 || classes[0] == Amd64ArgClass::X87Up ||
      classes[1] == Amd64ArgClass::X87 || classes[1] == Amd64ArgClass::X87Up ||
      classes[0] == Amd64ArgClass::ComplexX87 ||
      classes[1] == Amd64ArgClass::ComplexX87) {
    set_memory(classes);
  }
}

}  // namespace

int amd64_type_size_bytes(const TypeSpec& ts, const Module& mod) {
  using c4c::TypeBase;
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return 8;
  if (ts.array_rank > 0) {
    TypeSpec elem = drop_one_array_dim(ts);
    int elem_size = amd64_type_size_bytes(elem, mod);
    if (elem_size <= 0) return 0;
    int count = array_elem_count(ts);
    if (count <= 0) return 0;
    return elem_size * count;
  }
  if ((ts.base == TypeBase::TB_STRUCT || ts.base == TypeBase::TB_UNION) &&
      ts.tag && ts.tag[0]) {
    if (const HirStructDef* sd = lookup_struct(mod, ts)) return sd->size_bytes;
    return 0;
  }
  if (ts.base == TypeBase::TB_VA_LIST) return kAmd64VaListStructBytes;
  return primitive_size_bytes(ts.base);
}

bool amd64_type_is_aggregate(const TypeSpec& ts) {
  if (ts.array_rank > 0) return true;
  using c4c::TypeBase;
  return (ts.base == TypeBase::TB_STRUCT || ts.base == TypeBase::TB_UNION);
}

bool amd64_fixed_aggregate_passed_byval(const TypeSpec& ts, const Module& mod) {
  using c4c::TypeBase;
  if (ts.ptr_level > 0 || ts.array_rank > 0) return false;
  if (!(ts.base == TypeBase::TB_STRUCT || ts.base == TypeBase::TB_UNION)) return false;
  const auto layout = classify_amd64_vararg(ts, mod);
  return layout.size_bytes > 0 && layout.needs_memory;
}

bool aarch64_fixed_vector_passed_as_i32(const TypeSpec& ts, const Module& mod) {
  using namespace c4c::codegen::llvm_helpers;
  return llvm_target_is_aarch64(mod.target_profile) &&
         !llvm_target_is_apple(mod.target_profile) &&
         is_vector_value(ts) &&
         ts.vector_bytes == 4;
}

Amd64VarargInfo classify_amd64_vararg(const TypeSpec& ts, const Module& mod) {
  Amd64VarargInfo info;
  info.size_bytes = amd64_type_size_bytes(ts, mod);
  if (info.size_bytes <= 0) {
    info.needs_memory = true;
    return info;
  }
  std::array<Amd64ArgClass, 2> classes{Amd64ArgClass::None, Amd64ArgClass::None};
  classify_type(mod, ts, 0, classes);
  finalize_classes(classes, info.size_bytes);
  info.classes = classes;
  info.needs_memory = (classes[0] == Amd64ArgClass::Memory);
  if (!info.needs_memory) {
    for (Amd64ArgClass cls : classes) {
      if (cls == Amd64ArgClass::Integer) ++info.gp_chunks;
      if (cls == Amd64ArgClass::SSE) ++info.sse_slots;
    }
  }
  return info;
}

}  // namespace c4c::codegen::llvm_backend
