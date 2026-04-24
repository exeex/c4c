Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Public Parser Consumers

# Current Packet

## Just Finished

- Completed plan Step 1 inventory of non-parser-implementation
  `parser.hpp` include sites and direct `Parser`/parser-support usage.
- Inspected include sites with:
  `rg -n '#\s*include\s*[<"](?:.*?/)?parser\.hpp[">]' -S .`.
  Frontend parser include sites outside `src/frontend/parser/parser_*.cpp` are:
  `src/apps/c4cll.cpp`, `tests/frontend/frontend_parser_tests.cpp`,
  `tests/frontend/frontend_hir_tests.cpp`, `src/frontend/hir/hir_build.cpp`,
  `src/frontend/hir/hir_functions.cpp`, `src/frontend/hir/hir_types.cpp`, and
  `src/frontend/hir/hir_lowerer_internal.hpp`. Backend assembler matches under
  `src/backend/mir/*/assembler/` are unrelated local assembler headers named
  `parser.hpp`, not frontend parser consumers.
- Inspected direct use with:
  `rg -n '\bParser\b|parser\.' src/frontend/hir src/apps tests/frontend -S`,
  targeted reads of `src/frontend/parser/parser.hpp`,
  `src/frontend/parser/impl/parser_impl.hpp`, `src/apps/c4cll.cpp`,
  `tests/frontend/frontend_hir_tests.cpp`, `src/frontend/hir/hir_lowerer_internal.hpp`,
  and HIR/parser-support helper searches.
- Public production API that must remain reachable from `parser.hpp`:
  constructor `Parser(...)`, `parse()`, parse debug channel constants
  `ParseDebugNone`, `ParseDebugAll`, `ParseDebugTentative`,
  `ParseDebugInjected`, and `set_parser_debug_channels(...)`.
- Production accidental implementation access:
  `src/apps/c4cll.cpp` reads `parser.diagnostic_state_.had_error` directly.
  This is not a real public parser API; the facade needs a public diagnostic
  accessor before `ParserDiagnosticState` can be hidden.
- HIR consumers are not `Parser` object consumers. They include `parser.hpp`
  for parser-support free functions: `sizeof_type_spec(...)`,
  `alignof_type_spec(...)`, `sizeof_base(...)`, `eval_const_int(...)`,
  `resolve_typedef_chain(...)`, and `types_compatible_p(...)`. Direct HIR
  includes in `hir_build.cpp` and `hir_types.cpp` are currently redundant
  because `hir_lowerer_internal.hpp` already includes `parser.hpp`.
- `tests/frontend/frontend_hir_tests.cpp` is public test API: it uses only
  construction plus `parse()`.
- `tests/frontend/frontend_parser_tests.cpp` is intentional debug/test API:
  public entry `Parser(...)`/`parse()` plus `SymbolId`, `kInvalidSymbol`,
  `QualifiedNameRef`, `TemplateArgParseResult`, `TemplateScopeKind`,
  `TypenameTemplateParamKind`, `symbol_id_for_token(...)`,
  `symbol_spelling(...)`, `token_spelling(...)`, `make_injected_token(...)`,
  `make_node(...)`, `parse_qualified_name(...)`, `apply_qualified_name(...)`,
  `peek_qualified_name(...)`, `is_clearly_value_template_arg(...)`,
  `capture_template_arg_expr(...)`, `is_type_start()`,
  `looks_like_unresolved_identifier_type_head(...)`,
  `parser_text_id_for_token(...)`, `find_parser_text_id(...)`,
  `ensure_named_namespace_context(...)`, `current_namespace_context_id()`,
  `intern_semantic_name_key(...)`, `alias_template_key_in_context(...)`,
  `find_alias_template_info_in_context(...)`,
  `register_concept_name_in_context(...)`, `is_concept_name(...)`,
  `skip_attributes()`, `skip_exception_spec()`, `parse_attributes(...)`,
  `parse_base_type()`, `parse_unary()`, `parse_dependent_typename_specifier(...)`,
  `decode_type_ref_text(...)`, `has_conflicting_user_typedef_binding(...)`,
  `begin_record_body_context(...)`, `resolve_visible_type_name(...)`,
  `resolve_visible_value_name(...)`, `resolve_typedef_type_chain(...)`,
  `parse_stmt()`, `parse_top_level()`, `find_template_struct_primary(...)`,
  `try_parse_template_type_arg(...)`,
  `eval_deferred_nttp_expr_tokens(...)`,
  `classify_typename_template_parameter()`,
  `consume_declarator_post_pointer_qualifiers()`,
  `parse_qualified_declarator_name(...)`, `save_state()`,
  `restore_state(...)`, `push_local_binding_scope()`,
  `bind_local_typedef(...)`, `bind_local_value(...)`,
  `pop_local_binding_scope()`, `register_typedef_binding(...)`,
  `register_var_type_binding(...)`, `register_synthesized_typedef_binding(...)`,
  `unregister_typedef_binding(...)`, `has_typedef_name(...)`,
  `has_typedef_type(...)`, `find_typedef_type(...)`, `has_var_type(...)`,
  `find_var_type(...)`, `find_visible_typedef_type(...)`,
  `find_visible_var_type(...)`, `is_typedef_name(...)`,
  `set_last_using_alias_name(...)`, `last_using_alias_name_text()`,
  `clear_last_using_alias_name()`, `clear_last_resolved_typedef()`, `cur()`,
  `pos_`, and `tokens_`.
- `tests/frontend/frontend_parser_tests.cpp` accidental implementation access:
  direct mutation/inspection of `shared_lookup_state_`, `binding_state_`,
  `active_context_state_`, `definition_state_`, `namespace_state_`, and
  `template_state_`.
- Parser support declarations that are not public `Parser` facade members and
  can leave `parser.hpp` in a first mechanical slice:
  parser/HIR support declarations should move to a narrow support header
  (`sizeof_base`, `sizeof_type_spec`, `alignof_type_spec`, `eval_const_int`,
  `resolve_typedef_chain`, `types_compatible_p`); parser-only support
  declarations can move behind `impl/parser_impl.hpp`
  (`eval_enum_expr`, `is_dependent_enum_expr`, `is_qualifier`,
  `is_storage_class`, `is_type_kw`, `lexeme_is_imaginary`,
  `parse_int_lexeme`, `parse_float_lexeme`, and likely `align_base` plus
  `effective_scalar_base` unless kept as support-header implementation detail).
- Facade/PIMPL blocker: `parser.hpp` cannot stop including
  `impl/parser_state.hpp` while the public `Parser` object contains state
  fields by value/reference (`core_input_state_`, `tokens_`, `pos_`, `arena_`,
  `shared_lookup_state_`, `binding_state_`, `definition_state_`,
  `template_state_`, `lexical_scope_state_`, `active_context_state_`,
  `namespace_state_`, `diagnostic_state_`, `pragma_state_`) and while tests
  directly inspect/mutate those fields. Hiding this state requires a facade
  accessor/PIMPL-style slice or a deliberate test-only internal header route.

## Suggested Next

- First code-changing sub-slice: split parser support declarations out of
  `parser.hpp` before attempting state hiding. Add a narrow public support
  header for HIR-used helpers, make HIR include that support header instead of
  `parser.hpp`, and move parser-only support declarations under
  `impl/parser_impl.hpp`. Keep behavior unchanged and run the focused parser
  build/test proof from `plan.md`.

## Watchouts

- Do not treat frontend HIR support-helper use as a reason to keep all of
  `Parser` public. HIR needs helper declarations, not the `Parser` class.
- `src/apps/c4cll.cpp` needs a public diagnostic result/accessor before
  `diagnostic_state_` can become private or move behind a state handle.
- `tests/frontend/frontend_parser_tests.cpp` intentionally exercises parser
  internals. A later state-hiding slice should decide whether those tests move
  to an internal parser test header, keep temporary debug/test accessors, or
  remain a known blocker.
- C++ member declarations for split parser implementation methods cannot move
  out of the `Parser` class without changing the object model. The feasible
  near-term move is support/free declarations and implementation-index
  commentary/includes; real state hiding is a separate facade/PIMPL-shaped
  slice.

## Proof

- No build/test run; supervisor delegated an inventory-only packet.
- Proof rationale: only `todo.md` changed. Inventory was based on `rg` include
  and symbol-use searches plus targeted reads of the exact consumer and header
  files named above, so no compile proof was required and no `test_after.log`
  was created for this packet.
