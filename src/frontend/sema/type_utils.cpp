#include "type_utils.hpp"

#include <cctype>
#include <cstdint>
#include <cstring>

namespace c4c {

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

bool is_lvalue_ref_ty(const TypeSpec& ts) {
  return ts.is_lvalue_ref;
}

TypeSpec remove_lvalue_ref(TypeSpec ts) {
  ts.is_lvalue_ref = false;
  return ts;
}

TypeSpec add_lvalue_ref(TypeSpec ts) {
  ts.is_lvalue_ref = true;
  return ts;
}

bool is_rvalue_ref_ty(const TypeSpec& ts) {
  return ts.is_rvalue_ref;
}

TypeSpec remove_rvalue_ref(TypeSpec ts) {
  ts.is_rvalue_ref = false;
  return ts;
}

TypeSpec add_rvalue_ref(TypeSpec ts) {
  ts.is_rvalue_ref = true;
  return ts;
}

bool is_any_ref_ty(const TypeSpec& ts) {
  return ts.is_lvalue_ref || ts.is_rvalue_ref;
}

TypeSpec remove_any_ref(TypeSpec ts) {
  ts.is_lvalue_ref = false;
  ts.is_rvalue_ref = false;
  return ts;
}

TypeSpec ref_storage_type(TypeSpec ts) {
  if (!ts.is_lvalue_ref && !ts.is_rvalue_ref) return ts;
  ts.ptr_level += 1;
  return ts;
}

bool is_vector_ty(const TypeSpec& ts) {
  return ts.is_vector && ts.vector_lanes > 0 && ts.ptr_level == 0 && ts.array_rank == 0;
}

void clear_vector(TypeSpec& ts) {
  ts.is_vector = false;
  ts.vector_lanes = 0;
  ts.vector_bytes = 0;
}

void set_vector_type(TypeSpec& ts, long long lanes, long long total_bytes) {
  ts.array_size = -1;
  ts.array_rank = 0;
  for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
  ts.is_ptr_to_array = false;
  ts.inner_rank = -1;
  ts.array_size_expr = nullptr;
  ts.is_vector = lanes > 0;
  ts.vector_lanes = ts.is_vector ? lanes : 0;
  ts.vector_bytes = ts.is_vector ? total_bytes : 0;
}

TypeSpec vector_element_type(TypeSpec ts) {
  clear_vector(ts);
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

TypeSpec decay_array_to_ptr(TypeSpec ts) {
  if (is_vector_ty(ts)) return ts;
  if (!is_array_ty(ts)) return ts;
  ts = drop_array_dim(ts);
  ts.ptr_level += 1;
  if (is_array_ty(ts)) ts.is_ptr_to_array = true;
  return ts;
}

TypeSpec classify_int_literal_type(Node* n) {
  if (!n) return int_ts();
  if (n->is_imaginary) return make_ts(TB_COMPLEX_LONGLONG);
  const char* sv = n->sval;
  bool is_hex_or_oct = sv && sv[0] == '0' && (sv[1] == 'x' || sv[1] == 'X' ||
                                               (sv[1] >= '0' && sv[1] <= '7'));
  if (sv) {
    size_t len = strlen(sv);
    int lcount = 0;
    bool has_u = false;
    for (int i = (int)len - 1; i >= 0; i--) {
      char c = sv[i];
      if (c == 'l' || c == 'L') {
        lcount++;
      } else if (c == 'u' || c == 'U') {
        has_u = true;
      } else if (c == 'i' || c == 'I' || c == 'j' || c == 'J') {
      } else {
        break;
      }
    }
    if (lcount >= 2) return has_u ? make_ts(TB_ULONGLONG) : make_ts(TB_LONGLONG);
    if (lcount == 1) return has_u ? make_ts(TB_ULONG) : make_ts(TB_LONG);
    if (has_u) return make_ts(TB_UINT);
  }
  long long v = n->ival;
  if (is_hex_or_oct) {
    if (v >= 0 && v <= 0x7fffffff) return int_ts();
    if ((unsigned long long)v <= 0xffffffff) return make_ts(TB_UINT);
    if (v >= 0 && v <= (long long)0x7fffffffffffffff) return make_ts(TB_LONGLONG);
    return make_ts(TB_ULONGLONG);
  }
  return int_ts();
}

TypeSpec classify_float_literal_type(Node* n) {
  if (!n) return double_ts();
  const char* sv = n->sval;
  const bool is_f16 = sv &&
      (strstr(sv, "f16") != nullptr || strstr(sv, "F16") != nullptr);
  const bool is_f64 = sv &&
      (strstr(sv, "f64") != nullptr || strstr(sv, "F64") != nullptr);
  const bool is_f128 = sv &&
      (strstr(sv, "f128") != nullptr || strstr(sv, "F128") != nullptr);
  const bool is_bf16 = sv &&
      (strstr(sv, "bf16") != nullptr || strstr(sv, "BF16") != nullptr);
  const bool has_fixed_width_suffix = is_f16 || is_f64 || is_f128 || is_bf16;
  const bool is_f32 = sv &&
      (strstr(sv, "f32") != nullptr || strstr(sv, "F32") != nullptr ||
       (!has_fixed_width_suffix &&
        (strchr(sv, 'f') != nullptr || strchr(sv, 'F') != nullptr)));
  const bool is_ldbl = sv && (strchr(sv, 'l') != nullptr || strchr(sv, 'L') != nullptr);
  if (n->is_imaginary) {
    if (is_f16 || is_f32 || is_bf16) return make_ts(TB_COMPLEX_FLOAT);
    if (is_f128 || is_ldbl) return make_ts(TB_COMPLEX_LONGDOUBLE);
    if (is_f64) return make_ts(TB_COMPLEX_DOUBLE);
    return make_ts(TB_COMPLEX_DOUBLE);
  }
  if (is_f16 || is_f32 || is_bf16) return float_ts();
  if (is_f64) return double_ts();
  if (is_f128) return make_ts(TB_LONGDOUBLE);
  if (is_ldbl) return make_ts(TB_LONGDOUBLE);
  return double_ts();
}

TypeSpec classify_unary_result_type(const char* op, const TypeSpec& operand, int operand_size) {
  if (!op) return int_ts();
  if (strcmp(op, "!") == 0) return int_ts();
  if ((strcmp(op, "~") == 0 || strcmp(op, "-") == 0 || strcmp(op, "+") == 0) &&
      operand.ptr_level == 0 && !is_array_ty(operand) && !is_float_base(operand.base) &&
      !is_complex_base(operand.base) && operand_size < 4)
    return int_ts();
  return operand;
}

static bool is_compare_or_logical_op(const char* op) {
  if (!op) return false;
  return strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 || strcmp(op, "<") == 0 ||
         strcmp(op, "<=") == 0 || strcmp(op, ">") == 0 || strcmp(op, ">=") == 0 ||
         strcmp(op, "&&") == 0 || strcmp(op, "||") == 0;
}

static TypeSpec usual_arithmetic_result_type(const TypeSpec& lhs, const TypeSpec& rhs) {
  if (lhs.base == TB_LONGDOUBLE || rhs.base == TB_LONGDOUBLE) return make_ts(TB_LONGDOUBLE);
  if (lhs.base == TB_DOUBLE || rhs.base == TB_DOUBLE) return double_ts();
  if (lhs.base == TB_FLOAT || rhs.base == TB_FLOAT) return float_ts();
  if (lhs.base == TB_ULONGLONG || rhs.base == TB_ULONGLONG) return make_ts(TB_ULONGLONG);
  if (lhs.base == TB_LONGLONG || rhs.base == TB_LONGLONG) {
    if (lhs.base == TB_ULONG || rhs.base == TB_ULONG) return make_ts(TB_ULONGLONG);
    return ll_ts();
  }
  if (lhs.base == TB_ULONG || rhs.base == TB_ULONG) return make_ts(TB_ULONG);
  if (lhs.base == TB_LONG || rhs.base == TB_LONG) return make_ts(TB_LONG);
  if (lhs.base == TB_UINT || rhs.base == TB_UINT) return make_ts(TB_UINT);
  return int_ts();
}

TypeSpec classify_binop_result_type(
    const char* op,
    const TypeSpec& lhs,
    const TypeSpec& rhs,
    int lhs_size,
    int rhs_size) {
  if (!op) return int_ts();
  if (is_compare_or_logical_op(op)) return int_ts();
  if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) {
    if (lhs.ptr_level > 0) return int_ts();
    if (lhs.ptr_level == 0 && !is_array_ty(lhs) && !is_float_base(lhs.base) &&
        !is_complex_base(lhs.base) && lhs_size < 4)
      return int_ts();
    return lhs;
  }
  if (is_vector_ty(lhs)) return lhs;
  if (is_vector_ty(rhs)) return rhs;
  if (is_array_ty(lhs) && lhs.ptr_level == 0 && !lhs.is_ptr_to_array &&
      is_array_ty(rhs) && rhs.ptr_level == 0 && !rhs.is_ptr_to_array &&
      array_rank_of(lhs) == 1 && array_rank_of(rhs) == 1 &&
      !is_compare_or_logical_op(op))
    return lhs;
  if ((lhs.ptr_level > 0 || is_array_ty(lhs)) &&
      (rhs.ptr_level > 0 || is_array_ty(rhs)) &&
      strcmp(op, "-") == 0)
    return ll_ts();
  if (lhs.ptr_level > 0) return lhs;
  if (is_array_ty(lhs)) return decay_array_to_ptr(lhs);
  if (rhs.ptr_level > 0) return rhs;
  if (is_array_ty(rhs)) return decay_array_to_ptr(rhs);
  if (is_complex_base(lhs.base) || is_complex_base(rhs.base)) {
    if (is_complex_base(lhs.base) && is_complex_base(rhs.base))
      return (lhs_size >= rhs_size) ? lhs : rhs;
    return is_complex_base(lhs.base) ? lhs : rhs;
  }
  return usual_arithmetic_result_type(lhs, rhs);
}

TypeSpec classify_known_call_return_type(const char* callee_name, bool* known);

TypeSpec classify_known_builtin_return_type(BuiltinId id, bool* known) {
  if (known) *known = true;
  if (const BuiltinInfo* builtin = builtin_by_id(id)) {
    if (builtin->category == BuiltinCategory::AliasCall &&
        !builtin->canonical_name.empty()) {
      return classify_known_call_return_type(
          std::string(builtin->canonical_name).c_str(), known);
    }
  }
  switch (builtin_result_kind(id)) {
    case BuiltinResultKind::Void:
      return void_ts();
    case BuiltinResultKind::Pointer:
      return ptr_to(TB_VOID);
    case BuiltinResultKind::UShort:
      return make_ts(TB_USHORT);
    case BuiltinResultKind::UInt:
      return make_ts(TB_UINT);
    case BuiltinResultKind::ULongLong:
      return make_ts(TB_ULONGLONG);
    case BuiltinResultKind::Float:
      return float_ts();
    case BuiltinResultKind::Double:
      return double_ts();
    case BuiltinResultKind::LongDouble:
      return make_ts(TB_LONGDOUBLE);
    case BuiltinResultKind::ComplexFloat:
      return make_ts(TB_COMPLEX_FLOAT);
    case BuiltinResultKind::ComplexDouble:
      return make_ts(TB_COMPLEX_DOUBLE);
    case BuiltinResultKind::ComplexLongDouble:
      return make_ts(TB_COMPLEX_LONGDOUBLE);
    case BuiltinResultKind::Int:
      return make_ts(TB_INT);
    case BuiltinResultKind::Unknown:
      if (known) *known = false;
      return int_ts();
  }
  if (known) *known = false;
  return int_ts();
}

TypeSpec classify_known_call_return_type(const char* callee_name, bool* known) {
  if (known) *known = true;
  if (!callee_name) return int_ts();
  switch (known_call_result_kind(callee_name)) {
    case BuiltinResultKind::Pointer:
      return ptr_to(TB_VOID);
    case BuiltinResultKind::Void:
      return void_ts();
    default:
      break;
  }
  if (known) *known = false;
  return int_ts();
}

namespace {

bool same_optional_cstr(const char* lhs, const char* rhs) {
  if (lhs == rhs) return true;
  if (!lhs || !rhs) return false;
  return std::strcmp(lhs, rhs) == 0;
}

bool same_template_arg_ref_list(const TemplateArgRefList& lhs,
                                const TemplateArgRefList& rhs);

bool same_template_arg_ref(const TemplateArgRef& lhs, const TemplateArgRef& rhs) {
  if (lhs.kind != rhs.kind) return false;
  if (lhs.kind == TemplateArgKind::Value) {
    return lhs.value == rhs.value;
  }
  return type_binding_values_equivalent(lhs.type, rhs.type);
}

bool same_template_arg_ref_list(const TemplateArgRefList& lhs,
                                const TemplateArgRefList& rhs) {
  if (lhs.size != rhs.size) return false;
  if (lhs.data == rhs.data) return true;
  if (lhs.size <= 0) return true;
  if (!lhs.data || !rhs.data) return false;
  for (int i = 0; i < lhs.size; ++i) {
    if (!same_template_arg_ref(lhs.data[i], rhs.data[i])) return false;
  }
  return true;
}

}  // namespace

bool type_binding_values_equivalent(const TypeSpec& lhs, const TypeSpec& rhs) {
  if (lhs.base != rhs.base) return false;
  if (lhs.enum_underlying_base != rhs.enum_underlying_base) return false;
  if (!same_optional_cstr(lhs.tag, rhs.tag)) return false;
  if (lhs.n_qualifier_segments != rhs.n_qualifier_segments) return false;
  for (int i = 0; i < lhs.n_qualifier_segments; ++i) {
    const char* lseg = lhs.qualifier_segments ? lhs.qualifier_segments[i] : nullptr;
    const char* rseg = rhs.qualifier_segments ? rhs.qualifier_segments[i] : nullptr;
    if (!same_optional_cstr(lseg, rseg)) return false;
  }
  if (lhs.is_global_qualified != rhs.is_global_qualified) return false;
  if (lhs.ptr_level != rhs.ptr_level) return false;
  if (lhs.is_lvalue_ref != rhs.is_lvalue_ref) return false;
  if (lhs.is_rvalue_ref != rhs.is_rvalue_ref) return false;
  if (lhs.align_bytes != rhs.align_bytes) return false;
  if (lhs.array_size != rhs.array_size) return false;
  if (lhs.array_rank != rhs.array_rank) return false;
  for (int i = 0; i < 8; ++i) {
    if (lhs.array_dims[i] != rhs.array_dims[i]) return false;
  }
  if (lhs.is_ptr_to_array != rhs.is_ptr_to_array) return false;
  if (lhs.inner_rank != rhs.inner_rank) return false;
  if (lhs.is_vector != rhs.is_vector) return false;
  if (lhs.vector_lanes != rhs.vector_lanes) return false;
  if (lhs.vector_bytes != rhs.vector_bytes) return false;
  if (lhs.array_size_expr != rhs.array_size_expr) return false;
  if (lhs.is_const != rhs.is_const) return false;
  if (lhs.is_volatile != rhs.is_volatile) return false;
  if (lhs.is_fn_ptr != rhs.is_fn_ptr) return false;
  if (lhs.is_packed != rhs.is_packed) return false;
  if (lhs.is_noinline != rhs.is_noinline) return false;
  if (lhs.is_always_inline != rhs.is_always_inline) return false;
  if (!same_optional_cstr(lhs.tpl_struct_origin, rhs.tpl_struct_origin)) return false;
  if (!same_template_arg_ref_list(lhs.tpl_struct_args, rhs.tpl_struct_args)) return false;
  if (lhs.deferred_member_type_text_id != kInvalidText &&
      rhs.deferred_member_type_text_id != kInvalidText) {
    if (lhs.deferred_member_type_text_id != rhs.deferred_member_type_text_id) {
      return false;
    }
  } else if (!same_optional_cstr(lhs.deferred_member_type_name,
                                 rhs.deferred_member_type_name)) {
    return false;
  }
  return true;
}

bool is_wide_str_lit(Node* n) {
  if (!n || n->kind != NK_STR_LIT || !n->sval) return false;
  const char* s = n->sval;
  return (s[0] == 'L' || s[0] == 'u' || s[0] == 'U') && s[1] == '"';
}

int str_lit_byte_len(Node* n) {
  if (!n || n->kind != NK_STR_LIT || !n->sval || is_wide_str_lit(n)) return -1;
  const char* raw = n->sval;
  int start = 0;
  if (raw[start] == '"')
    start++;
  else
    return -1;
  int len = 0;
  for (const char* p = raw + start; *p && *p != '"'; p++) {
    if (*p == '\\' && *(p + 1)) {
      char esc = *(p + 1);
      if ((esc >= '0' && esc <= '7')) {
        p++;
        while (*(p + 1) >= '0' && *(p + 1) <= '7') p++;
      } else if (esc == 'x' || esc == 'X') {
        p++;
        while (isxdigit((unsigned char)*(p + 1))) p++;
      } else {
        p++;
      }
    }
    len++;
  }
  return len + 1;
}

bool allows_string_literal_ptr_target(const TypeSpec& ts) {
  if (ts.ptr_level <= 0) return false;
  switch (ts.base) {
    case TB_CHAR:
    case TB_SCHAR:
    case TB_UCHAR:
    case TB_VOID:
      return true;
    default:
      return false;
  }
}

std::string decode_narrow_string_lit(const char* sval) {
  if (!sval) return "";
  std::string raw = sval;
  std::string content;
  if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"') {
    for (size_t i = 1; i + 1 < raw.size(); i++) {
      if (raw[i] == '\\' && i + 1 < raw.size() - 1) {
        char esc = raw[++i];
        switch (esc) {
          case 'n':
            content += '\n';
            break;
          case 't':
            content += '\t';
            break;
          case 'r':
            content += '\r';
            break;
          case '0':
            content += '\0';
            break;
          case '\\':
            content += '\\';
            break;
          case '"':
            content += '"';
            break;
          case 'x':
          case 'X': {
            unsigned int v = 0;
            int nhex = 0;
            while (i + 1 < raw.size() - 1 &&
                   isxdigit((unsigned char)raw[i + 1])) {
              char h = raw[++i];
              v = v * 16 +
                  (isdigit((unsigned char)h) ? h - '0'
                                             : (tolower((unsigned char)h) - 'a' + 10));
              nhex++;
            }
            if (nhex == 0)
              content += 'x';
            else
              content += (char)(unsigned char)v;
            break;
          }
          default:
            if (esc >= '0' && esc <= '7') {
              unsigned int v = (unsigned int)(esc - '0');
              int cnt = 1;
              while (cnt < 3 && i + 1 < raw.size() - 1 && raw[i + 1] >= '0' &&
                     raw[i + 1] <= '7') {
                v = v * 8 + (unsigned int)(raw[++i] - '0');
                cnt++;
              }
              content += (char)(unsigned char)v;
            } else {
              content += esc;
            }
            break;
        }
      } else {
        content += raw[i];
      }
    }
  } else {
    content = raw;
  }
  return content;
}

std::string normalize_printf_longdouble_format(std::string s) {
  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] != '%') {
      out.push_back(s[i]);
      continue;
    }
    if (i + 1 < s.size() && s[i + 1] == '%') {
      out += "%%";
      i++;
      continue;
    }
    size_t j = i + 1;
    bool saw_L = false;
    std::string spec = "%";
    while (j < s.size()) {
      char c = s[j];
      if (c == 'L') {
        saw_L = true;
        j++;
        continue;
      }
      spec.push_back(c);
      if (isalpha((unsigned char)c) || c == '%') {
        if (saw_L &&
            (c == 'f' || c == 'F' || c == 'e' || c == 'E' || c == 'g' ||
             c == 'G' || c == 'a' || c == 'A')) {
          out += spec;
        } else {
          if (saw_L) out.push_back('%');
          if (saw_L) {
            for (size_t k = i + 1; k <= j; k++)
              if (s[k] != 'L') out.push_back(s[k]);
          } else {
            out += spec;
          }
        }
        break;
      }
      j++;
    }
    i = j;
  }
  return out;
}

std::vector<uint32_t> decode_wide_string(const char* sval) {
  std::vector<uint32_t> codepoints;
  if (!sval) {
    codepoints.push_back(0);
    return codepoints;
  }
  const char* p = sval;
  while (*p && *p != '"') p++;
  if (*p == '"') p++;
  while (*p && *p != '"') {
    if (*p == '\\') {
      p++;
      switch (*p) {
        case 'n':
          codepoints.push_back('\n');
          p++;
          break;
        case 't':
          codepoints.push_back('\t');
          p++;
          break;
        case 'r':
          codepoints.push_back('\r');
          p++;
          break;
        case '0':
          codepoints.push_back(0);
          p++;
          break;
        case 'a':
          codepoints.push_back('\a');
          p++;
          break;
        case 'b':
          codepoints.push_back('\b');
          p++;
          break;
        case 'f':
          codepoints.push_back('\f');
          p++;
          break;
        case 'v':
          codepoints.push_back('\v');
          p++;
          break;
        case '\\':
          codepoints.push_back('\\');
          p++;
          break;
        case '"':
          codepoints.push_back('"');
          p++;
          break;
        case '\'':
          codepoints.push_back('\'');
          p++;
          break;
        case 'x':
        case 'X': {
          p++;
          uint32_t v = 0;
          while (*p && isxdigit((unsigned char)*p)) {
            v = v * 16 + (isdigit((unsigned char)*p) ? *p - '0'
                                                     : tolower((unsigned char)*p) - 'a' + 10);
            p++;
          }
          codepoints.push_back(v);
          break;
        }
        case 'u': {
          p++;
          uint32_t v = 0;
          for (int k = 0; k < 4 && *p && isxdigit((unsigned char)*p); k++, p++) {
            v = v * 16 + (isdigit((unsigned char)*p) ? *p - '0'
                                                     : tolower((unsigned char)*p) - 'a' + 10);
          }
          codepoints.push_back(v);
          break;
        }
        case 'U': {
          p++;
          uint32_t v = 0;
          for (int k = 0; k < 8 && *p && isxdigit((unsigned char)*p); k++, p++) {
            v = v * 16 + (isdigit((unsigned char)*p) ? *p - '0'
                                                     : tolower((unsigned char)*p) - 'a' + 10);
          }
          codepoints.push_back(v);
          break;
        }
        default:
          if (*p >= '0' && *p <= '7') {
            uint32_t v = 0;
            for (int k = 0; k < 3 && *p >= '0' && *p <= '7'; k++, p++) v = v * 8 + (*p - '0');
            codepoints.push_back(v);
          } else {
            codepoints.push_back((unsigned char)*p);
            p++;
          }
          break;
      }
    } else {
      unsigned char c0 = (unsigned char)*p;
      if (c0 < 0x80) {
        codepoints.push_back(c0);
        p++;
      } else if ((c0 & 0xE0) == 0xC0) {
        uint32_t v = c0 & 0x1F;
        if (p[1] && (unsigned char)p[1] >= 0x80) {
          v = (v << 6) | ((unsigned char)p[1] & 0x3F);
          p += 2;
        } else {
          p++;
        }
        codepoints.push_back(v);
      } else if ((c0 & 0xF0) == 0xE0) {
        uint32_t v = c0 & 0x0F;
        if (p[1] && (unsigned char)p[1] >= 0x80) {
          v = (v << 6) | ((unsigned char)p[1] & 0x3F);
          p += 2;
        } else {
          p++;
          codepoints.push_back(v);
          continue;
        }
        if (*p && (unsigned char)*p >= 0x80) {
          v = (v << 6) | ((unsigned char)*p & 0x3F);
          p++;
        }
        codepoints.push_back(v);
      } else if ((c0 & 0xF8) == 0xF0) {
        uint32_t v = c0 & 0x07;
        for (int k = 0; k < 3 && p[1] && (unsigned char)p[1] >= 0x80; k++, p++) {
          v = (v << 6) | ((unsigned char)p[1] & 0x3F);
        }
        p++;
        codepoints.push_back(v);
      } else {
        codepoints.push_back(c0);
        p++;
      }
    }
  }
  codepoints.push_back(0);
  return codepoints;
}

int infer_array_size_from_init(Node* init) {
  if (!init) return 0;
  if (init->kind == NK_STR_LIT) {
    if (is_wide_str_lit(init)) {
      auto codepoints = decode_wide_string(init->sval);
      return (int)codepoints.size();
    }
    int count = 0;
    for (int i = (init->sval && init->sval[0] == '"' ? 1 : 0);
         init->sval && init->sval[i] && init->sval[i] != '"'; i++) {
      if (init->sval[i] == '\\') {
        i++;
        if (init->sval[i] >= '0' && init->sval[i] <= '7') {
          for (int k = 0; k < 2 && init->sval[i + 1] >= '0' &&
                          init->sval[i + 1] <= '7';
               k++)
            i++;
        } else if (init->sval[i] == 'x' || init->sval[i] == 'X') {
          i++;
          while (init->sval[i] && isxdigit((unsigned char)init->sval[i])) i++;
          i--;
        }
      }
      count++;
    }
    return count + 1;
  }
  if (init->kind != NK_INIT_LIST) return 0;
  long long max_idx = -1;
  long long cur_idx = 0;
  for (int i = 0; i < init->n_children; i++) {
    Node* item = init->children[i];
    if (item && item->kind == NK_INIT_ITEM && item->is_index_desig) cur_idx = item->desig_val;
    if (cur_idx > max_idx) max_idx = cur_idx;
    cur_idx++;
  }
  return (int)(max_idx + 1);
}

long long static_eval_int(
    Node* n,
    const std::unordered_map<std::string, long long>& enum_consts) {
  if (!n) return 0;
  if (n->kind == NK_INT_LIT) return n->ival;
  if (n->kind == NK_CHAR_LIT) return n->ival;
  if (n->kind == NK_VAR && n->name) {
    auto it = enum_consts.find(n->name);
    if (it != enum_consts.end()) return it->second;
  }
  if (n->kind == NK_CAST && n->left) {
    long long v = static_eval_int(n->left, enum_consts);
    TypeSpec ts = n->type;
    if (ts.ptr_level == 0) {
      int bits = 0;
      switch (ts.base) {
        case TB_BOOL:
          bits = 1;
          break;
        case TB_CHAR:
        case TB_UCHAR:
        case TB_SCHAR:
          bits = 8;
          break;
        case TB_SHORT:
        case TB_USHORT:
          bits = 16;
          break;
        case TB_INT:
        case TB_UINT:
        case TB_ENUM:
          bits = 32;
          break;
        default:
          break;
      }
      if (bits > 0 && bits < 64) {
        long long mask = (1LL << bits) - 1;
        v &= mask;
        if (!is_unsigned_base(ts.base) && ts.base != TB_BOOL && (v >> (bits - 1)))
          v |= ~mask;
      }
    }
    return v;
  }
  if (n->kind == NK_UNARY && n->op) {
    if (strcmp(n->op, "-") == 0 && n->left) return -static_eval_int(n->left, enum_consts);
    if (strcmp(n->op, "+") == 0 && n->left) return static_eval_int(n->left, enum_consts);
    if (strcmp(n->op, "~") == 0 && n->left) return ~static_eval_int(n->left, enum_consts);
  }
  if (n->kind == NK_BINOP && n->op && n->left && n->right) {
    long long l = static_eval_int(n->left, enum_consts);
    long long r = static_eval_int(n->right, enum_consts);
    if (strcmp(n->op, "+") == 0) return l + r;
    if (strcmp(n->op, "-") == 0) return l - r;
    if (strcmp(n->op, "*") == 0) return l * r;
    if (strcmp(n->op, "/") == 0 && r != 0) return l / r;
    if (strcmp(n->op, "%") == 0 && r != 0) return l % r;
    if (strcmp(n->op, "&") == 0) return l & r;
    if (strcmp(n->op, "|") == 0) return l | r;
    if (strcmp(n->op, "^") == 0) return l ^ r;
    if (strcmp(n->op, "<<") == 0) return l << (r & 63);
    if (strcmp(n->op, ">>") == 0) return l >> (r & 63);
  }
  return 0;
}

double static_eval_float(Node* n) {
  if (!n) return 0.0;
  if (n->kind == NK_FLOAT_LIT) return n->fval;
  if (n->kind == NK_INT_LIT) return (double)n->ival;
  if (n->kind == NK_CHAR_LIT) return (double)n->ival;
  if (n->kind == NK_CAST && n->left) return static_eval_float(n->left);
  if (n->kind == NK_UNARY && n->op && n->left) {
    double v = static_eval_float(n->left);
    if (strcmp(n->op, "-") == 0) return -v;
    if (strcmp(n->op, "+") == 0) return v;
  }
  if (n->kind == NK_BINOP && n->op && n->left && n->right) {
    double l = static_eval_float(n->left);
    double r = static_eval_float(n->right);
    if (strcmp(n->op, "+") == 0) return l + r;
    if (strcmp(n->op, "-") == 0) return l - r;
    if (strcmp(n->op, "*") == 0) return l * r;
    if (strcmp(n->op, "/") == 0) return l / r;
  }
  return 0.0;
}

}  // namespace c4c
