Status: Active
Source Idea Path: ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Parser Record And Template Mirror Consumers

# Current Packet

## Just Finished

Step 1 inventory completed for the six parser rendered record/template mirrors.
Targeted text search was sufficient; clang AST queries were not needed because
the fields are concentrated in parser state plus a small set of helper bodies.

Authority map:

- `defined_struct_tags`: produced by direct record tag setup in
  `types/struct.cpp`, explicit/full template specialization reuse in
  `types/template.cpp`, and direct template emission in `types/base.cpp`.
  Consumers are `has_defined_struct_tag()`, record redefinition checks,
  constructor-like type recognition, qualified-owner probes, and template
  instantiation reuse tests. Structured carriers available nearby are
  `QualifiedNameKey`/`TextId` for names, namespace context ids, direct
  `Node*` record definitions, and `TypeSpec::record_def`. Safe uses are
  generated/final tag spelling, C compatibility, and TextId-less fallback.
  Code-changing need: semantic membership checks should stop treating the
  rendered tag set as authority when a structured type probe or `record_def`
  exists.
- `struct_tag_def_map`: produced by record registration, test fixtures,
  explicit specialization reuse, injected template parse, and direct template
  emission. Consumers include const-int/offsetof evaluation, base/member
  typedef lookup, template static member lookup, injected instantiation lookup,
  and compatibility record lookups in `parse_base_type()`. Structured carriers
  available nearby are `TypeSpec::record_def`, direct `Node*` selected template
  pattern, `QualifiedNameKey`/`TextId`, namespace context, and parser symbol
  identity. Safe uses are final rendered tag compatibility, HIR/codegen bridge
  spelling, const evaluator fallback, and testing mirrors. Code-changing need:
  call sites that already hold `TypeSpec` or selected `Node*` should call
  `resolve_record_type_spec()`/use `record_def` first and only consult the map
  as an explicit fallback with mismatch visibility.
- `template_struct_defs`: produced by `register_template_struct_primary()` as
  rendered primary and rendered qualified bridge entries. Consumers are
  `find_template_struct_primary()` fallback paths and parser tests that verify
  demotion. Structured carrier is `template_struct_defs_by_key` keyed by
  `QualifiedNameKey`; `TextId` and namespace context are present at normal
  registration and lookup sites. Safe uses are TextId-less compatibility,
  bridge spelling, and final/debug spelling. Current code is already
  structured-primary for valid `TextId`; remaining code-changing work is to
  keep any rendered fallback visible and avoid adding new semantic lookups
  through this map.
- `template_struct_specializations`: produced by
  `register_template_struct_specialization()` as rendered primary-name and
  rendered qualified-primary mirrors. Consumers are
  `find_template_struct_specializations()` fallback paths and specialization
  selection through that helper. Structured carrier is
  `template_struct_specializations_by_key` keyed by `QualifiedNameKey`; direct
  `primary_tpl` also carries namespace context and unqualified `TextId`.
  Safe uses are TextId-less compatibility and rendered qualified bridge
  fallback. Current code is structured-primary for normal parser-owned names;
  code-changing work should focus on making fallback contracts explicit rather
  than treating rendered specialization vectors as authoritative.
- `instantiated_template_struct_keys`: produced by
  `mark_template_instantiation_dedup_keys()` in `types/template.cpp` and
  direct-emission marking in `types/base.cpp`; synchronized/healed by the
  matching `sync`/`has` helpers. Consumers are template instantiation reuse and
  direct template emission dedup. Structured carrier is
  `instantiated_template_struct_keys_by_key` keyed by
  `TemplateInstantiationKey` with `QualifiedNameKey` plus canonical argument
  keys. Safe uses are rendered compatibility and healing the old mirror.
  Current code has mismatch counters and structured-primary behavior when a
  structured key exists; remaining code-changing work is to prevent legacy-only
  rendered keys from suppressing concrete emission in structured-keyable paths.
- `nttp_default_expr_tokens`: produced by template parameter attachment through
  `cache_nttp_default_expr_tokens()`. Consumer is
  `eval_deferred_nttp_default()`. Structured carrier is
  `nttp_default_expr_tokens_by_key` keyed by `NttpDefaultExprKey`
  (`QualifiedNameKey`, param index). Safe uses are TextId-less/default replay
  compatibility and mismatch comparison. Current code is structured-primary
  when the template name has a valid `TextId`; remaining code-changing work is
  limited to preserving that rule and not widening rendered-key authority.

## Suggested Next

First code-changing packet target: Step 2, narrow the record-definition lookup
surface by converting the remaining `defined_struct_tags`/`struct_tag_def_map`
semantic consumers that already hold `TypeSpec` or direct `Node*` data to
structured-primary lookup. Start with `resolves_to_record_ctor_type()` and the
qualified-owner/member-typedef lookup paths, because they currently combine
`TypeSpec::record_def`-capable probes with rendered tag membership/map fallback.

Recommended proof command for the supervisor to delegate after that packet:
`cmake --build build --target frontend_parser_tests && ctest --test-dir build -R '^frontend_parser_tests$' --output-on-failure`

## Watchouts

Do not delete rendered maps yet. They still serve TextId-less compatibility,
final tag spelling, generated template instance spelling, const-evaluator
fallback, and downstream bridge behavior. The risky areas are not the template
primary/specialization helpers, which already prefer `QualifiedNameKey`, but
record consumers that fall back to `defined_struct_tags` or
`struct_tag_def_map` even after a `TypeSpec::record_def` or selected template
`Node*` is available.

Existing parser tests around `frontend_parser_tests.cpp` already cover
structured-primary template lookup, NTTP default cache demotion, instantiation
dedup mirrors, direct emission, and record-definition payload preservation.
Step 2 should add or extend nearby namespace-qualified/colliding-record cases
rather than proving only one stale-map fixture.

## Proof

Read-only inventory packet. Per delegation, no build or tests were run and no
`test_after.log` was produced.
