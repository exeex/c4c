#include "sema_driver.hpp"

namespace tinyc2ll::frontend_cxx {

TypeSpec make_ts(TypeBase base) {
  TypeSpec ts{};
  ts.base = base;
  ts.array_size = -1;
  ts.array_rank = 0;
  for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
  ts.is_ptr_to_array = false;
  ts.inner_rank = -1;
  return ts;
}

TypeSpec int_ts() { return make_ts(TB_INT); }
TypeSpec void_ts() { return make_ts(TB_VOID); }
TypeSpec double_ts() { return make_ts(TB_DOUBLE); }
TypeSpec float_ts() { return make_ts(TB_FLOAT); }
TypeSpec char_ts() { return make_ts(TB_CHAR); }
TypeSpec ll_ts() { return make_ts(TB_LONGLONG); }

TypeSpec ptr_to(TypeBase base) {
  TypeSpec ts = make_ts(base);
  ts.ptr_level = 1;
  return ts;
}

int array_rank_of(const TypeSpec& ts) {
  if (ts.array_rank > 0) return ts.array_rank;
  if (ts.array_size != -1) return 1;
  return 0;
}

long long array_dim_at(const TypeSpec& ts, int idx) {
  if (ts.array_rank > 0) {
    if (idx >= 0 && idx < ts.array_rank) return ts.array_dims[idx];
    return -1;
  }
  if (idx == 0 && ts.array_size != -1) return ts.array_size;
  return -1;
}

bool is_array_ty(const TypeSpec& ts) {
  return array_rank_of(ts) > 0;
}

void clear_array(TypeSpec& ts) {
  ts.array_size = -1;
  ts.array_rank = 0;
  for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
  ts.is_ptr_to_array = false;
}

void set_array_dims(TypeSpec& ts, const long long* dims, int rank) {
  clear_array(ts);
  if (rank <= 0) return;
  if (rank > 8) rank = 8;
  ts.array_rank = rank;
  for (int i = 0; i < rank; ++i) ts.array_dims[i] = dims[i];
  ts.array_size = ts.array_dims[0];
}

void set_first_array_dim(TypeSpec& ts, long long dim0) {
  int rank = array_rank_of(ts);
  if (rank <= 0) {
    long long dims[1] = {dim0};
    set_array_dims(ts, dims, 1);
    return;
  }
  long long dims[8];
  for (int i = 0; i < rank && i < 8; ++i) dims[i] = array_dim_at(ts, i);
  dims[0] = dim0;
  set_array_dims(ts, dims, rank);
}

TypeSpec drop_array_dim(TypeSpec ts) {
  int rank = array_rank_of(ts);
  if (rank <= 0) {
    clear_array(ts);
    return ts;
  }
  long long dims[8];
  int out = 0;
  for (int i = 1; i < rank && out < 8; ++i) dims[out++] = array_dim_at(ts, i);
  set_array_dims(ts, dims, out);
  if (ts.inner_rank >= 0) {
    int new_rank = out;
    if (new_rank == ts.inner_rank) {
      ts.is_ptr_to_array = true;
      ts.inner_rank = -1;
    }
  }
  return ts;
}

bool is_unsigned_base(TypeBase b) {
  return b == TB_UCHAR || b == TB_USHORT || b == TB_UINT ||
         b == TB_ULONG || b == TB_ULONGLONG || b == TB_BOOL;
}

bool is_float_base(TypeBase b) {
  return b == TB_FLOAT || b == TB_DOUBLE || b == TB_LONGDOUBLE;
}

bool is_complex_base(TypeBase b) {
  switch (b) {
    case TB_COMPLEX_FLOAT:
    case TB_COMPLEX_DOUBLE:
    case TB_COMPLEX_LONGDOUBLE:
    case TB_COMPLEX_CHAR:
    case TB_COMPLEX_SCHAR:
    case TB_COMPLEX_UCHAR:
    case TB_COMPLEX_SHORT:
    case TB_COMPLEX_USHORT:
    case TB_COMPLEX_INT:
    case TB_COMPLEX_UINT:
    case TB_COMPLEX_LONG:
    case TB_COMPLEX_ULONG:
    case TB_COMPLEX_LONGLONG:
    case TB_COMPLEX_ULONGLONG:
      return true;
    default:
      return false;
  }
}

TypeSpec complex_component_ts(TypeBase b) {
  switch (b) {
    case TB_COMPLEX_FLOAT:
      return make_ts(TB_FLOAT);
    case TB_COMPLEX_LONGDOUBLE:
      return make_ts(TB_LONGDOUBLE);
    case TB_COMPLEX_DOUBLE:
      return make_ts(TB_DOUBLE);
    case TB_COMPLEX_CHAR:
      return make_ts(TB_CHAR);
    case TB_COMPLEX_SCHAR:
      return make_ts(TB_SCHAR);
    case TB_COMPLEX_UCHAR:
      return make_ts(TB_UCHAR);
    case TB_COMPLEX_SHORT:
      return make_ts(TB_SHORT);
    case TB_COMPLEX_USHORT:
      return make_ts(TB_USHORT);
    case TB_COMPLEX_INT:
      return make_ts(TB_INT);
    case TB_COMPLEX_UINT:
      return make_ts(TB_UINT);
    case TB_COMPLEX_LONG:
      return make_ts(TB_LONG);
    case TB_COMPLEX_ULONG:
      return make_ts(TB_ULONG);
    case TB_COMPLEX_LONGLONG:
      return make_ts(TB_LONGLONG);
    case TB_COMPLEX_ULONGLONG:
      return make_ts(TB_ULONGLONG);
    default:
      return make_ts(TB_DOUBLE);
  }
}

}  // namespace tinyc2ll::frontend_cxx
