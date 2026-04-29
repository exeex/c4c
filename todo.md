# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Re-Inventory Parser String Lookup After Record Bridge

## Just Finished

Step 1 inventory completed after the typed parser record identity bridge.

Focused search found the remaining parser-owned `std::string` map/set families:

- `ParserDefinitionState::defined_struct_tags`: compatibility/final-spelling
  tag presence mirror.
- `ParserDefinitionState::struct_tag_def_map`: compatibility/final-spelling
  rendered tag-to-record mirror. `resolve_record_type_spec()` now prefers
  `TypeSpec::record_def` before consulting this map.
- `ParserTemplateState::template_struct_defs` and
  `template_struct_specializations`: legacy rendered-name mirrors beside the
  `QualifiedNameKey` primary/specialization tables.
- `ParserTemplateState::instantiated_template_struct_keys`: legacy rendered
  template-instantiation de-dup mirror beside `TemplateInstantiationKey`.
- `ParserTemplateState::nttp_default_expr_tokens`: legacy rendered NTTP default
  token cache beside `NttpDefaultExprKey`.
- Function-local parser helpers:
  `knr_param_decl_map`, record `field_names_seen`, template match
  `type_bindings_map`/`value_bindings_map`, alias substitution `subst`, and
  type/NTTP binding vectors. These are local text-spelling algorithm state, not
  global semantic authority.

Retained compatibility/helper categories:

- Record layout const-eval helpers in `parser_support.hpp` and `support.cpp`
  keep `std::unordered_map<std::string, Node*>* struct_map` so
  `sizeof`/`alignof`/`offsetof` can still evaluate tag-only compatibility
  inputs, while typed paths use `record_def` first.
- The `eval_const_int()` overload taking
  `std::unordered_map<std::string, long long>* compatibility_named_consts`
  remains a rendered-name bridge. Parser-owned const-int evaluation already
  routes named constants through `TextId`.
- Template rendered maps remain compatibility mirrors only when lookup lacks a
  valid `TextId`/structured key; existing tests assert stale rendered names do
  not override structured lookup.
- Tests at `frontend_parser_tests.cpp:3721` and `:3767` are retained
  compatibility coverage for typed record layout preference and tag-only
  fallback.

Unresolved downstream boundary:

- `resolve_typedef_chain()` / `types_compatible_p()` still accept
  `std::unordered_map<std::string, TypeSpec>& tmap`. The focused search found
  these as public support helper signatures rather than parser-owned state; do
  not fold them into the first conversion packet without a caller-specific
  route.

## Suggested Next

First bounded conversion packet: convert the function-local K&R parameter
declaration lookup in `parse_declaration_or_function()` from
`std::unordered_map<std::string, Node*> knr_param_decl_map` to a parser text
identity lookup keyed by `TextId`.

Scope for that packet:

- Intern K&R declaration names and identifier-list names at the local lookup
  boundary.
- Keep `Node::name` spelling unchanged for diagnostics/final AST text.
- Do not touch `struct_tag_def_map`, record layout const-eval, template
  rendered mirrors, or public support helper signatures.
- Focus proof on building plus the frontend parser test subset that exercises
  old-style/K&R parameter declarations.

## Watchouts

Do not treat `defined_struct_tags` or `struct_tag_def_map` as semantic
conversion targets in the next packet. They are compatibility/final-spelling
mirrors after idea 127.

Template legacy maps already have structured mirrors and stale-rendered-name
tests; converting or renaming them is a separate semantic/compatibility packet,
not the smallest Step 2 text-map slice.

The `resolve_typedef_chain()` / `types_compatible_p()` `tmap` signatures may
need a downstream support-helper idea if a future packet finds live parser
callers that require structured typedef-chain compatibility.

## Proof

Inventory-only packet; no build required by delegated proof.

Focused searches run:

- `rg -n "unordered_map<\\s*std::string|std::map<\\s*std::string|map<\\s*std::string" src/frontend/parser tests/frontend`
- `rg -n "std::(unordered_)?set<\\s*std::string|std::vector<std::pair<std::string|std::unordered_map<std::string" src/frontend/parser/impl src/frontend/parser/parser.hpp src/frontend/parser/parser_support.hpp tests/frontend/frontend_parser_tests.cpp`
- `rg -n "template_struct_defs\\b|template_struct_specializations\\b|instantiated_template_struct_keys\\b|nttp_default_expr_tokens\\b|nttp_default_expr_cache_mismatch|template_struct_instantiation_key_mismatch" src/frontend/parser/impl src/frontend/parser/parser.hpp tests/frontend/frontend_parser_tests.cpp`
- `rg -n "eval_const_int\\(|resolve_typedef_chain\\(|types_compatible_p\\(|compatibility_named_consts|struct_map" src/frontend/parser tests/frontend/frontend_parser_tests.cpp`

Result: inventory complete. No `test_after.log` was generated because this
packet was explicitly inventory-only with no build/test proof required.
