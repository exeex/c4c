#pragma once

// Private parser implementation index. Keep this as a compatibility bridge
// while parser-private declarations move out of the public parser header.
#include "parser_state.hpp"
#include "../parser_support.hpp"
#include "../parser.hpp"

namespace c4c {

bool eval_enum_expr(Node* n, const ParserEnumConstTable& consts, long long* out);
bool is_dependent_enum_expr(Node* n, const ParserEnumConstTable& consts);
TypeBase effective_scalar_base(const TypeSpec& ts);
long long align_base(TypeBase b, int ptr_level);

bool is_qualifier(TokenKind k);
bool is_storage_class(TokenKind k);
bool is_type_kw(TokenKind k);

bool lexeme_is_imaginary(const char* s);
long long parse_int_lexeme(const char* s);
double parse_float_lexeme(const char* s);

}  // namespace c4c
