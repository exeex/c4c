# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Parser String Authority

## Just Finished

Completed Step 1 inventory for parser legacy string lookup authority.

Parser-owned string-keyed lookup maps and sets:
- `ParserDefinitionState::defined_struct_tags` and `struct_tag_def_map` in
  `src/frontend/parser/impl/parser_state.hpp` are rendered tag authority for
  record definition lookup. Named tags usually have `TextId` available through
  parser tokens or AST `unqualified_text_id`, but anonymous/template-mangled
  tags and `eval_const_int` struct-map callers still make this an unresolved
  downstream bridge rather than the first safe conversion.
- `ParserTemplateState::template_struct_defs`,
  `template_struct_specializations`, `instantiated_template_struct_keys`, and
  `nttp_default_expr_tokens` are rendered-name mirrors beside structured
  `QualifiedNameKey`, `TemplateInstantiationKey`, and
  `NttpDefaultExprKey` tables. These are compatibility/final-spelling mirrors;
  lookup should stay structured-first and only fall back on TextId-less bridge
  paths.
- Local K&R parameter maps, field-name duplicate sets, template substitution
  maps, temporary type/value binding maps, and type-compatibility `tmap`
  parameters are packet-local spelling/algorithm data, not durable parser
  lookup authority.

Helper overloads and semantic lookup paths:
- Typedef/value/concept/function helpers in `impl/core.cpp` already prefer
  `TextId`, `SymbolId`, `LocalNameKey`, or `QualifiedNameKey` storage through
  `ParserNameTables`, `ParserBindingState`, `ParserLexicalScopeState`, and
  namespace tables. String overloads such as `has_typedef_type`,
  `find_typedef_type`, `register_typedef_binding`, `find_var_type`,
  `register_var_type_binding`, and `register_known_fn_name` are compatibility
  input bridges.
- `lookup_type_in_context`, `lookup_value_in_context`,
  `lookup_concept_in_context`, `lookup_using_value_alias`,
  `find_visible_typedef_type`, and `find_visible_var_type` are the main
  semantic lookup paths. Existing tests expect valid-`TextId` misses not to be
  rescued by corrupted rendered fallback names.
- `bridge_name_in_context`, `compatibility_namespace_name_in_context`,
  `render_structured_name`, and `render_value_binding_name` are display and
  bridge rendering surfaces, not primary authority.
- `eval_const_int` has a primary `TextId` named-constant overload and a
  rendered-name compatibility overload. Struct size/align/offsetof still use
  rendered struct tags through `struct_tag_def_map`.

Relevant parser tests:
- `tests/frontend/frontend_parser_tests.cpp` covers symbol/TextId wrappers,
  ID-first lookup, qualified string bridges, using aliases/imports,
  namespace value demotion, visible type/value lookup, local binding tables,
  synthesized typedef unregister, template lookup demotion, NTTP default
  cache mirrors, template-instantiation dedup mirrors, and alias-template
  structured-key preference.
- Generated parser disambiguation fixtures under
  `tests/cpp/internal/generated/parser_disambiguation_matrix/` cover qualified
  owner/type spelling across declaration contexts.
- Focused parse/frontend cases under `tests/cpp/internal/postive_case/parse/`
  and `tests/cpp/internal/postive_case/frontend/` exercise qualified names,
  using aliases, typedef lookup, concepts, operators, templates, and namespace
  and member contexts.

## Suggested Next

First bounded implementation packet: demote the parser template rendered-name
mirrors in `ParserTemplateState` by making their names and call sites explicit
compatibility mirrors, then tighten `find_template_struct_primary`,
`find_template_struct_specializations`, `eval_deferred_nttp_default`, and
template-instantiation dedup paths so valid-`TextId`/structured-key misses
never promote legacy rendered entries. Keep mirror writes for compatibility
and final spelling; do not touch record tag maps in this packet.

Focused proof command for that packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`

## Watchouts

Do not convert `struct_tag_def_map` first: struct tags feed sizeof/alignof,
offsetof, template instantiation, and downstream type surfaces through rendered
`TypeSpec::tag` strings. Treat that as a later semantic-record packet or a
separate bridge blocker if it requires HIR/LIR changes.

Do not delete string overloads wholesale. Public/parser test helpers still use
string inputs as compatibility bridges, and many token-spelling probes are
source spelling or diagnostics rather than lookup authority.

Overfit risk: changing tests by weakening legacy bridge coverage would violate
the plan. The first packet should preserve TextId-less compatibility while
proving valid structured misses are not rescued by rendered-name mirrors.

## Proof

Inventory-only packet. No build or tests run, per delegation.

Future implementation proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
