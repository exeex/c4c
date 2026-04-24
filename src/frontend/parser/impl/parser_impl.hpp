#pragma once

// Private parser implementation index for parser_*.cpp translation units.
// This header gathers declarations shared inside the parser implementation
// without extending the public parser facade.
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
