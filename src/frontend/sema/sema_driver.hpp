#pragma once

#include "ast.hpp"

namespace tinyc2ll::frontend_cxx {

// Phase-1 semantic facade extracted from IRBuilder.
// Keep function signatures simple so legacy IRBuilder call sites remain stable.
TypeSpec make_ts(TypeBase base);
TypeSpec int_ts();
TypeSpec void_ts();
TypeSpec double_ts();
TypeSpec float_ts();
TypeSpec char_ts();
TypeSpec ll_ts();
TypeSpec ptr_to(TypeBase base);

int array_rank_of(const TypeSpec& ts);
long long array_dim_at(const TypeSpec& ts, int idx);
bool is_array_ty(const TypeSpec& ts);
void clear_array(TypeSpec& ts);
void set_array_dims(TypeSpec& ts, const long long* dims, int rank);
void set_first_array_dim(TypeSpec& ts, long long dim0);
TypeSpec drop_array_dim(TypeSpec ts);

bool is_unsigned_base(TypeBase b);
bool is_float_base(TypeBase b);
bool is_complex_base(TypeBase b);
TypeSpec complex_component_ts(TypeBase b);

}  // namespace tinyc2ll::frontend_cxx
