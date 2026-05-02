# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 repaired the general expression parser path for chained template-id
static-member references. `parse_assign_expr` now continues through repeated
`::member` segments after `Template<Args>::member`, so expressions such as
`Outer<T>::Count::value + 1` produce a complete structured `NK_VAR`/`NK_BINOP`
carrier instead of truncating at `Outer<T>::Count`.

`base.cpp` now uses that normal parser rematerialization path for
`captured_expr_tokens` when populating
`TemplateArgRef::type.array_size_expr`; it does not contain a local
token-shape parser. Focused parser coverage in
`frontend_parser_lookup_authority_tests` verifies that
`Buffer<Outer<T>::Count::value + 1>` carries a structured value expression with
qualifier/member `TextId`s and no `$expr:` debug-text authority.

HIR materialization still needs separate consumer work for nested-owner static
member value evaluation: the concrete `Wrap<int>` probe still lowers
`Buffer<Outer<T>::Count::value + 1>` as `Buffer_V_0` after the parser carrier
repair. Per the post-commit review in
`review/step4_post_nested_expr_carrier_review.md`, this is now parked under
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` rather than
continued as idea 139 Step 4 work. Idea 139 keeps the parser/Sema producer
repair; HIR value-argument consumer migration must be handled by idea 140 or a
narrower HIR metadata idea after a lifecycle switch.

## Suggested Next

Continue Step 4 only with parser/Sema-owned handoff work. The next executor
packet should re-inventory remaining parser-to-Sema metadata drops for typedef,
value, record, template, static-member, and consteval-reference handoff, then
pick the smallest route whose producer and consumer are both in
`src/frontend/parser` or `src/frontend/sema`. The packet must either preserve
an existing structured carrier through that parser/Sema boundary or record a
separate metadata blocker; it must not edit HIR.

The nested-owner static-member value-argument materialization gap is parked for
idea 140: when that idea is active, the HIR packet should consume the
parser-produced `NK_VAR` qualifier/template-arg carriers without rendered owner
parsing and should delete or narrow rendered HIR fallback authority on the
covered path.

## Watchouts

- The source idea remains active; Step 4 progress is not source-idea closure.
- Parser static-member references now canonicalize nested owner paths only when
  the path resolves through structured record/type metadata or nested
  `TypeSpec::record_def` carriers. Namespace-qualified ordinary values should
  continue through the normal qualified value path.
- `SourceLanguage` is declaration provenance, not linkage. Keep
  `linkage_spec` authoritative for explicit `extern "C"` conflict behavior.
- Manually constructed AST tests default zero-initialized nodes to C source
  language unless they are created through `Parser::make_node`.
- Static-member lookup still depends on parser/Sema owner and member metadata.
  A generated `NK_VAR` for `Owner::member` must carry `qualifier_text_ids`,
  `unqualified_text_id`, and the correct owner namespace context when the
  parser has the owner identity; do not restore rendered owner lookup to cover
  missing carriers.
- The remaining `$expr:` re-lex branch is compatibility-only for value args
  with no `ParsedTemplateArg::expr`, no value-arg `TypeSpec::array_size_expr`,
  no `ParsedTemplateArg::nttp_text_id`, and no `captured_expr_tokens`.
- `lookup_rendered_nttp_compatibility()` is still needed for older consteval
  NTTP parameter references that have no NTTP structured/TextId binding
  carrier. Do not broaden the gate to all local/named const metadata misses
  without first promoting those references to structured NTTP metadata.
- Some existing consteval producers still call `bind_consteval_call_env()` or
  `resolve_type()` with flat `type_bindings` only. This packet intentionally
  preserves that flat-only compatibility path; removing it requires a separate
  handoff packet that supplies structured/TextId type-binding channels at those
  producers.
- `captured_expr_tokens` is parser-local structured metadata. Do not promote
  stale `nttp_name`/`$expr:` text back to semantic authority when the token
  carrier exists.
- `tpl_struct_origin` remains compatibility/display spelling. Do not use it to
  rediscover template primaries or decide Sema `TypeSpec` equivalence when
  `tpl_struct_origin_key` or structured arg carriers exist.
- Simple type-only pending template args now use `TemplateArgRef` structure.
  The broad direct nested template-origin path must still stay on the legacy
  display route for now: promoting `Outer<Inner<T>>`-style pending args to
  typed nested `TemplateArgRef` carriers in the direct parser branch triggers
  existing HIR `owner struct still pending` failures in
  `template_struct_advanced.cpp` and `template_struct_nested.cpp`.
- Do not reintroduce rendered qualified-text parsing, `$expr:` debug-text
  semantic authority, string/string_view semantic lookup parameters, fallback
  spelling, expectation downgrades, or named-test shortcuts.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.
- Nested static-member owner metadata still uses Sema's existing owner key:
  owner namespace context plus final owner `TextId`. This packet repairs the
  parser carrier for available nested record identity; it does not add an
  enclosing-record path to the Sema key.
- The removed HIR nested-owner evaluator was mostly carrier-driven but is not
  part of idea 139. The parser/Sema producer is now repaired; HIR consumer
  work for nested-owner value-argument materialization belongs to idea 140 or
  a narrower HIR metadata blocker after a lifecycle switch.
- The `cpp_positive_sema_template_nttp_default_runtime_cpp` proof case covers
  the default-NTTP side of this path: static-member value expressions must carry
  enclosing type bindings when evaluating defaults such as
  `is_void<T>::value || is_void<T>::value`.
- Parked blocker for idea 140: HIR still does not evaluate the
  parser-produced nested-owner value carrier for materialization. Do not
  broaden `$expr:` compatibility, edit HIR under idea 139, or reintroduce the
  rejected parser mini-parser to cover that consumer gap.

## Proof

Ran the delegated proof command after the general expression-parser repair:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp|cpp_hir_template_struct_arg_materialization|cpp_hir_template_value_arg_static_member_trait)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `888/888` matched tests.
Final proof log path: `test_after.log`.
