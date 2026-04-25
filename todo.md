Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Current Parser Lookup Ownership

# Current Packet

## Just Finished

Step 1 - Inspect Current Parser Lookup Ownership completed. Current parser
lookup ownership map:
- Typedef names/types: local lexical visibility is already `TextId` native via
  `ParserLexicalScopeState::visible_typedef_types`; non-symbol fallback storage
  is `TextId` keyed in `ParserBindingState::non_atom_typedef*`; qualified/member
  typedef storage is `QualifiedNameKey` keyed in `struct_typedefs`; legacy
  rendered-name mirrors still live in `parser_name_tables.typedef*`.
- Value bindings: local lexical visibility is `TextId` native via
  `visible_value_types`; non-symbol fallback storage is `TextId` keyed in
  `non_atom_var_types`; qualified value lookup still renders a
  `QualifiedNameKey` to a string and probes `has_var_type` / `find_var_type`
  through `parser_name_tables.var_types`.
- Using value aliases: `UsingValueAlias::target_key` already carries structured
  identity; `compatibility_name` is still the rendered bridge used when the
  structured name cannot render and for existing string-result overloads.
- Template primary/specialization lookup: structured maps
  `template_struct_defs_by_key` and `template_struct_specializations_by_key`
  are dual-written with legacy rendered-string maps `template_struct_defs` and
  `template_struct_specializations`.
- NTTP default-expression cache: `nttp_default_expr_tokens` is still keyed by
  rendered `template-name:param-index` strings.
- Template instantiation de-dup: `instantiated_template_struct_keys` is still a
  rendered-string set, used near template-id parsing.

Bridge paths that must stay rendered strings for this run:
- `Node::name`, `TypeSpec::tag`, instantiated struct names, function/global
  names, diagnostics, link/codegen-facing names, and HIR/sema-facing map keys.
- `compatibility_namespace_name_in_context`, `bridge_name_in_context`,
  `visible_name_spelling`, `qualified_name_text`, and final
  `compatibility_spelling` outputs.
- Legacy template primary/specialization string mirrors, NTTP string cache mirror,
  and value/typedef string overloads while downstream callers still depend on
  rendered names.

## Suggested Next

First narrow implementation packet: Step 2 - Add Structured Value Binding
Storage. Add `QualifiedNameKey` value-binding storage in
`ParserBindingState`, plus structured `has/find/register` helpers in
`parser.hpp` and `impl/core.cpp`. Dual-write from `register_var_type_binding`
when a qualified/context key is available, keep existing string tables
unchanged, and add mismatch checks only where both structured and legacy lookup
results are naturally available.

Owned files for that packet:
- `src/frontend/parser/impl/parser_state.hpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/core.cpp`
- `tests/frontend/frontend_parser_tests.cpp`
- `todo.md`

Proposed proof command for that packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

## Watchouts

- Do not remove or demote `find_var_type(const std::string&)`,
  `has_var_type(const std::string&)`, or `register_var_type_binding` in the
  first implementation packet.
- The first code packet should not retarget `using` import behavior yet; keep it
  to structured storage/helpers plus proof-facing tests.
- Preserve downstream rendered-name bridge behavior and compatibility spellings.
- Use dual-read / dual-write proof before demoting any parser-owned string lookup
  path.
- Do not downgrade tests or add testcase-shaped lookup shortcuts.

## Proof

Inspection-only packet; no build required by delegation. `test_after.log` was
not touched.
