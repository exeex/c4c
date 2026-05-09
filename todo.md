# Current Packet

Status: Active
Source Idea Path: ideas/open/156_parser_support_constexpr_type_helper_domain_tables.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory parser-support string-map helper contracts

## Just Finished

Step 1 inventory inspected the string-map helper contracts in
`src/frontend/parser/parser_support.hpp` and their definitions in
`src/frontend/parser/impl/support.cpp`:
`resolve_record_type_spec`, both `eval_const_int` overloads,
`resolve_typedef_chain`, and `types_compatible_p`.

Direct caller groups:
- Structured record carriers: parser type helpers call
  `resolve_record_type_spec(..., nullptr)` after typedef/template resolution
  where `TypeSpec::record_def`, `tag_text_id`, namespace context, or qualifier
  TextIds may already exist.
- Parser compatibility record maps: template static-member layout uses
  `definition_state_.struct_tag_def_map` only for TextId-less legacy layout
  carriers; tests also exercise this bridge directly.
- Structured named constants: parser-owned constant folding uses
  `binding_state_.const_int_bindings` as a `TextId` table through
  `eval_const_int_with_parser_tables` and template static-member initializer
  evaluation.
- Rendered-only HIR compatibility: HIR template/static-member helpers still
  call `eval_const_int`, `resolve_typedef_chain`, and `types_compatible_p` with
  rendered string maps or empty rendered maps.
- Parser type compatibility: parser expression paths call
  `Parser::are_types_compatible`, which resolves typedefs through parser-owned
  structured tables instead of the public string-map helper.

Structured-metadata-miss fallback findings:
- Record layout paths already route through
  `resolve_record_type_spec_for_constant_layout`; complete structured
  `record_def` wins, and TextId/namespace/qualifier carriers do not recover
  through the rendered map after a structured miss.
- Public `resolve_record_type_spec` still has an intentional compatibility map
  path for parser probes without direct `record_def`; it rejects map recovery
  when structured context or TextId matching fails.
- `eval_const_int` has a primary `TextId` named-constant overload, but the
  rendered-name overload remains a compatibility bridge for HIR/non-parser
  callers.
- `resolve_typedef_chain` and `types_compatible_p` remain the exposed gap:
  their public contracts accept only `unordered_map<string, TypeSpec>`, and the
  string-map overloads are not yet visibly marked as compatibility-only.

## Suggested Next

First implementation packet: add parser-support `TextId` typedef-domain
overloads for `resolve_typedef_chain` and `types_compatible_p`, keyed by
`std::unordered_map<TextId, TypeSpec>`, and mark the retained string-map
typedef/type-compatibility overloads as compatibility bridges. Keep HIR
rendered-only callers on the bridge, and add focused tests proving structured
typedef identity does not recover through stale rendered typedef maps.

## Watchouts

Do not route parser expression calls back through public string-map
`types_compatible_p`; `Parser::are_types_compatible` is already the structured
parser-owned path. Do not weaken the existing residual-metadata tests around
record map fallback. The next packet should avoid broad Sema redesign and keep
HIR rendered-only calls explicitly compatibility-scoped.

## Proof

Source inventory only; no build or tests run, and no `test_after.log` was
created because the delegated packet prohibited implementation edits and test
logs. Focused proof command for the next implementation packet:
`cmake --build build --target frontend_parser_tests cpp_hir_parser_support_residual_metadata_test && ctest --test-dir build -R 'frontend_parser_tests|cpp_hir_parser_support_residual_metadata' --output-on-failure`.
