#pragma once

// Private parser implementation index for parser_*.cpp translation units.
// This header gathers declarations shared inside the parser implementation
// without extending the public parser facade.
#include "../parser_support.hpp"
#include "../parser.hpp"

namespace c4c {

struct ParserImpl {
  ParserCoreInputState core_input_state;
  ParserSharedLookupState shared_lookup_state;
  ParserBindingState binding_state;
  ParserDefinitionState definition_state;
  ParserTemplateState template_state;
  ParserLexicalScopeState lexical_scope_state;
  ParserActiveContextState active_context_state;
  ParserNamespaceState namespace_state;
  ParserDiagnosticState diagnostic_state;
  ParserPragmaState pragma_state;

  ParserImpl(std::vector<Token> tokens, Arena& arena,
             TextTable* token_texts, FileTable* token_files,
             SourceProfile source_profile, std::string source_file)
      : core_input_state(std::move(tokens), arena, source_profile,
                         std::move(source_file)),
        shared_lookup_state(token_texts, token_files) {}
};

bool eval_enum_expr(Node* n, const ParserEnumConstTable& consts, long long* out);
bool is_dependent_enum_expr(Node* n, const ParserEnumConstTable& consts);
TypeBase effective_scalar_base(const TypeSpec& ts);
long long align_base(TypeBase b, int ptr_level);

bool is_qualifier(TokenKind k);
bool is_storage_class(TokenKind k);
bool is_type_kw(TokenKind k);
int bin_prec(TokenKind k);
Node* parse_lambda_expr(Parser& parser, int ln);
Node* parse_sizeof_pack_expr(Parser& parser, int ln);
Node* parse_new_expr(Parser& parser, int ln, bool global_qualified);

bool lexeme_is_imaginary(const char* s);
long long parse_int_lexeme(const char* s);
double parse_float_lexeme(const char* s);

void handle_pragma_pack(Parser& parser, const std::string& args);
void handle_pragma_gcc_visibility(Parser& parser, const std::string& args);
void handle_pragma_exec(Parser& parser, const std::string& args);

}  // namespace c4c
