#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.hpp"

namespace tinyc2ll::frontend_cxx {

// Phase-1 semantic facade for semantic/type utilities used by the frontend.
// Keep function signatures simple for parser/sema integration points.
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
TypeSpec decay_array_to_ptr(TypeSpec ts);
TypeSpec classify_int_literal_type(Node* n);
TypeSpec classify_float_literal_type(Node* n);
TypeSpec classify_unary_result_type(const char* op, const TypeSpec& operand, int operand_size);
TypeSpec classify_binop_result_type(
    const char* op,
    const TypeSpec& lhs,
    const TypeSpec& rhs,
    int lhs_size,
    int rhs_size);
std::string remap_builtin_et_name(std::string callee_name);
TypeSpec classify_known_call_return_type(const char* callee_name, bool* known);

bool is_wide_str_lit(Node* n);
int str_lit_byte_len(Node* n);
bool allows_string_literal_ptr_target(const TypeSpec& ts);
std::string decode_narrow_string_lit(const char* sval);
std::string normalize_printf_longdouble_format(std::string s);
std::vector<uint32_t> decode_wide_string(const char* sval);

int infer_array_size_from_init(Node* init);
long long static_eval_int(
    Node* n,
    const std::unordered_map<std::string, long long>& enum_consts);
double static_eval_float(Node* n);

}  // namespace tinyc2ll::frontend_cxx
