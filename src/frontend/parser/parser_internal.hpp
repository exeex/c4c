#pragma once

#include <string>
#include <unordered_map>

#include "ast.hpp"
#include "token.hpp"

namespace c4c {

bool eval_enum_expr(Node* n, const std::unordered_map<std::string, long long>& consts,
                    long long* out);
bool is_dependent_enum_expr(Node* n,
                            const std::unordered_map<std::string, long long>& consts);
long long sizeof_base(TypeBase b);
long long align_base(TypeBase b, int ptr_level);
bool eval_const_int(Node* n, long long* out,
                    const std::unordered_map<std::string, Node*>* struct_map = nullptr,
                    const std::unordered_map<std::string, long long>* named_consts = nullptr);

bool is_qualifier(TokenKind k);
bool is_storage_class(TokenKind k);
bool is_type_kw(TokenKind k);

bool lexeme_is_imaginary(const char* s);
long long parse_int_lexeme(const char* s);
double parse_float_lexeme(const char* s);

TypeSpec resolve_typedef_chain(TypeSpec ts,
                               const std::unordered_map<std::string, TypeSpec>& tmap);
bool types_compatible_p(TypeSpec a, TypeSpec b,
                        const std::unordered_map<std::string, TypeSpec>& tmap);

}  // namespace c4c
