#pragma once

#include <array>

#include "../../frontend/hir/hir_ir.hpp"

namespace c4c::codegen::llvm_backend {

enum class Amd64ArgClass {
  None,
  Memory,
  Integer,
  SSE,
  SSEUp,
  X87,
  X87Up,
  ComplexX87,
};

struct Amd64VarargInfo {
  std::array<Amd64ArgClass, 2> classes{Amd64ArgClass::None, Amd64ArgClass::None};
  int size_bytes = 0;
  int gp_chunks = 0;
  int sse_slots = 0;
  bool needs_memory = false;
};

Amd64VarargInfo classify_amd64_vararg(const c4c::TypeSpec& ts,
                                      const c4c::hir::Module& mod);

int amd64_type_size_bytes(const c4c::TypeSpec& ts,
                          const c4c::hir::Module& mod);

bool amd64_type_is_aggregate(const c4c::TypeSpec& ts);

bool amd64_fixed_aggregate_passed_byval(const c4c::TypeSpec& ts,
                                        const c4c::hir::Module& mod);

bool aarch64_fixed_vector_passed_as_i32(const c4c::TypeSpec& ts,
                                        const c4c::hir::Module& mod);

}  // namespace c4c::codegen::llvm_backend
