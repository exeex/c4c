#pragma once

// Private parser implementation index for parser impl/*.cpp translation units.
// This header gathers declarations shared inside the parser implementation
// without extending the public parser facade.
#include "../parser_support.hpp"
#include "../parser.hpp"
#include "parser_state.hpp"

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
Node* parse_enum(Parser& parser);
Node* parse_param(Parser& parser);
Node* parse_block(Parser& parser);
Node* parse_stmt(Parser& parser);
Node* parse_local_decl(Parser& parser);
Node* parse_top_level(Parser& parser);
Node* parse_static_assert_declaration(Parser& parser);
Node* parse_initializer(Parser& parser);
Node* parse_init_list(Parser& parser);
void parse_top_level_parameter_list(
    Parser& parser,
    std::vector<Node*>* out_params,
    std::vector<const char*>* out_knr_param_names,
    bool* out_variadic);
void parse_declarator_parameter_list(Parser& parser,
                                     std::vector<Node*>* out_params,
                                     bool* out_variadic);
void parse_parenthesized_function_pointer_suffix(
    Parser& parser,
    TypeSpec& ts, bool is_nested_fn_ptr,
    Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
    bool* out_fn_ptr_variadic,
    Node*** out_ret_fn_ptr_params, int* out_n_ret_fn_ptr_params,
    bool* out_ret_fn_ptr_variadic);
void parse_parenthesized_pointer_declarator_prefix(Parser& parser,
                                                   TypeSpec& ts);
void skip_parenthesized_pointer_declarator_array_chunks(Parser& parser);
bool parse_parenthesized_pointer_declarator_name(Parser& parser,
                                                 const char** out_name,
                                                 TextId* out_name_text_id);
bool try_parse_nested_parenthesized_pointer_declarator(
    Parser& parser,
    TypeSpec& ts, const char** out_name,
    Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
    bool* out_fn_ptr_variadic, TextId* out_name_text_id);
bool parse_parenthesized_pointer_declarator_inner(
    Parser& parser,
    TypeSpec& ts, const char** out_name,
    Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
    bool* out_fn_ptr_variadic, TextId* out_name_text_id);
long long parse_one_declarator_array_dim(Parser& parser, TypeSpec& ts);
void parse_declarator_array_suffixes(Parser& parser, TypeSpec& ts,
                                     std::vector<long long>* out_dims);
void apply_declarator_array_dims(Parser& parser, TypeSpec& ts,
                                 const std::vector<long long>& decl_dims);
void finalize_parenthesized_pointer_declarator(
    Parser& parser,
    TypeSpec& ts, bool is_nested_fn_ptr, std::vector<long long>* decl_dims,
    Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
    bool* out_fn_ptr_variadic,
    Node*** out_ret_fn_ptr_params, int* out_n_ret_fn_ptr_params,
    bool* out_ret_fn_ptr_variadic);
void parse_parenthesized_pointer_declarator(
    Parser& parser,
    TypeSpec& ts, const char** out_name,
    Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
    bool* out_fn_ptr_variadic,
    Node*** out_ret_fn_ptr_params, int* out_n_ret_fn_ptr_params,
    bool* out_ret_fn_ptr_variadic, TextId* out_name_text_id);
void parse_non_parenthesized_declarator(Parser& parser, TypeSpec& ts,
                                        const char** out_name);
void parse_non_parenthesized_declarator_tail(
    Parser& parser,
    TypeSpec& ts, const char** out_name,
    bool decay_plain_function_suffix, TextId* out_name_text_id);
void parse_plain_function_declarator_suffix(Parser& parser, TypeSpec& ts,
                                            bool decay_to_function_pointer);
bool try_parse_grouped_declarator(Parser& parser, TypeSpec& ts,
                                  const char** out_name,
                                  TextId* out_name_text_id,
                                  std::vector<long long>* out_dims);
void parse_normal_declarator_tail(Parser& parser, TypeSpec& ts,
                                  const char** out_name,
                                  TextId* out_name_text_id,
                                  std::vector<long long>* out_dims);
void parse_non_parenthesized_declarator_suffixes(
    Parser& parser, TypeSpec& ts, const char** out_name,
    TextId* out_name_text_id, std::vector<long long>* out_dims);
void parse_declarator_prefix(Parser& parser, TypeSpec& ts,
                             bool* out_is_parameter_pack);
void store_declarator_function_pointer_params(
    Parser& parser, Node*** out_params, int* out_n_params, bool* out_variadic,
    const std::vector<Node*>& params, bool variadic);
void apply_declarator_pointer_token(Parser& parser, TypeSpec& ts,
                                    TokenKind pointer_tok,
                                    bool preserve_array_base);
bool try_parse_declarator_member_pointer_prefix(Parser& parser, TypeSpec& ts);
bool parse_qualified_declarator_name(Parser& parser, std::string* out_name,
                                     TextId* out_name_text_id = nullptr);
void consume_declarator_post_pointer_qualifiers(Parser& parser);
void parse_pointer_ref_qualifiers(Parser& parser, TypeSpec& ts,
                                  TokenKind pointer_tok,
                                  bool preserve_array_base,
                                  bool consume_pointer_token = true);
bool is_grouped_declarator_start(Parser& parser);
bool is_parenthesized_pointer_declarator_start(Parser& parser);
bool try_parse_cpp_scoped_base_type(Parser& parser, bool already_have_base,
                                    TypeSpec* out_ts);
bool try_parse_qualified_base_type(Parser& parser, TypeSpec* out_ts);
Node* parse_expr(Parser& parser);
Node* parse_assign_expr(Parser& parser);
Node* parse_ternary(Parser& parser);
Node* parse_binary(Parser& parser, int min_prec);
Node* parse_unary(Parser& parser);
Node* parse_postfix(Parser& parser, Node* base);
Node* parse_primary(Parser& parser);

bool is_qualifier(TokenKind k);
bool is_storage_class(TokenKind k);
bool is_type_kw(TokenKind k);
int bin_prec(TokenKind k);
Node* parse_lambda_expr(Parser& parser, int ln);
Node* parse_sizeof_pack_expr(Parser& parser, int ln);
Node* parse_new_expr(Parser& parser, int ln, bool global_qualified);
const char* consume_adjacent_string_literal(Parser& parser);
bool is_record_special_member_name(Parser& parser, const std::string& lex,
                                   const std::string& struct_source_name);
bool try_skip_record_friend_member(Parser& parser);
bool try_skip_record_static_assert_member(Parser& parser,
                                          std::vector<Node*>* methods);
bool begin_record_member_parse(Parser& parser);
bool try_parse_record_access_label(Parser& parser);
Parser::RecordMemberRecoveryResult recover_record_member_parse_error(
    Parser& parser, int member_start_pos);
void parse_record_template_member_prelude(
    Parser& parser,
    std::vector<Parser::InjectedTemplateParam>* injected_type_params,
    bool* pushed_template_scope);
void parse_decl_attrs_for_record(Parser& parser, int line, TypeSpec* attr_ts);
void skip_record_base_specifier_tail(Parser& parser);
bool try_parse_record_base_specifier(Parser& parser, TypeSpec* base_ts);
void parse_record_base_clause(Parser& parser, std::vector<TypeSpec>* base_types);
bool try_parse_record_using_member(
    Parser& parser,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types);
bool try_parse_record_typedef_member(
    Parser& parser,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types);
bool try_parse_nested_record_member(
    Parser& parser,
    std::vector<Node*>* fields,
    const std::function<void(const char*)>& check_dup_field);
bool try_parse_record_enum_member(
    Parser& parser,
    std::vector<Node*>* fields,
    const std::function<void(const char*)>& check_dup_field);
bool try_parse_record_constructor_member(
    Parser& parser,
    const std::string& struct_source_name,
    std::vector<Node*>* methods);
bool try_parse_record_destructor_member(
    Parser& parser,
    const std::string& struct_source_name,
    std::vector<Node*>* methods);
bool try_parse_record_method_or_field_member(
    Parser& parser,
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    const std::function<void(const char*)>& check_dup_field);
bool try_parse_record_type_like_member_dispatch(
    Parser& parser,
    std::vector<Node*>* fields,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field);
bool try_parse_record_member_dispatch(
    Parser& parser,
    const std::string& struct_source_name,
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field);
bool try_parse_record_special_member_dispatch(
    Parser& parser,
    const std::string& struct_source_name,
    std::vector<Node*>* methods);
bool try_parse_record_member_with_template_prelude(
    Parser& parser,
    const std::string& struct_source_name,
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field);
bool try_parse_record_member_prelude(Parser& parser,
                                     std::vector<Node*>* methods);
bool try_parse_record_member(
    Parser& parser,
    const std::string& struct_source_name,
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field);
bool try_parse_record_body_member(
    Parser& parser,
    const std::string& struct_source_name,
    Parser::RecordBodyState* body_state,
    const std::function<void(const char*)>& check_dup_field);
void begin_record_body_context(Parser& parser,
                               const char* tag,
                               TextId tag_text_id,
                               const char* template_origin_name,
                               std::string* saved_struct_tag,
                               std::string* struct_source_name);
void parse_record_body(Parser& parser,
                       const std::string& struct_source_name,
                       Parser::RecordBodyState* body_state);
void parse_record_body_with_context(Parser& parser,
                                    const char* tag,
                                    TextId tag_text_id,
                                    const char* template_origin_name,
                                    Parser::RecordBodyState* body_state);
void finish_record_body_context(Parser& parser,
                                const std::string& saved_struct_tag);
void parse_record_definition_prelude(
    Parser& parser,
    int line,
    TypeSpec* attr_ts,
    const char** tag,
    TextId* tag_text_id,
    const char** template_origin_name,
    std::vector<Parser::TemplateArgParseResult>* specialization_args,
    std::vector<TypeSpec>* base_types);
Node* parse_record_tag_setup(
    Parser& parser,
    int line,
    bool is_union,
    const char** tag,
    const char* template_origin_name,
    const TypeSpec& attr_ts,
    const std::vector<Parser::TemplateArgParseResult>& specialization_args);
Node* build_record_definition_node(
    Parser& parser,
    int line,
    bool is_union,
    const char* tag,
    const char* template_origin_name,
    const TypeSpec& attr_ts,
    const std::vector<Parser::TemplateArgParseResult>& specialization_args,
    const std::vector<TypeSpec>& base_types);
Node* parse_record_definition_after_tag_setup(
    Parser& parser,
    int line,
    bool is_union,
    const char* tag,
    TextId tag_text_id,
    const char* template_origin_name,
    const TypeSpec& attr_ts,
    const std::vector<Parser::TemplateArgParseResult>& specialization_args,
    const std::vector<TypeSpec>& base_types);
void apply_record_trailing_type_attributes(Parser& parser, Node* sd);
void store_record_body_members(Parser& parser,
                               Node* sd,
                               const Parser::RecordBodyState& body_state);
void register_record_definition(Parser& parser,
                                Node* sd,
                                bool is_union,
                                const char* source_tag);
void finalize_record_definition(Parser& parser,
                                Node* sd,
                                bool is_union,
                                const char* source_tag,
                                const Parser::RecordBodyState& body_state);
void parse_record_definition_body(Parser& parser,
                                  Node* sd,
                                  bool is_union,
                                  const char* source_tag,
                                  const char* tag,
                                  TextId tag_text_id,
                                  const char* template_origin_name);
Node* parse_struct_or_union(Parser& parser, bool is_union);

bool lexeme_is_imaginary(const char* s);
long long parse_int_lexeme(const char* s);
double parse_float_lexeme(const char* s);

void handle_pragma_pack(Parser& parser, const std::string& args);
void handle_pragma_gcc_visibility(Parser& parser, const std::string& args);
void handle_pragma_exec(Parser& parser, const std::string& args);

}  // namespace c4c
