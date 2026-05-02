# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 addressed the reviewer rejection in
`review/step4_nested_value_arg_carrier_review.md` by removing the rejected
bespoke captured-token mini-parser from `base.cpp`, removing the speculative
nested-owner HIR evaluator from `value_args.cpp`, and restoring the focused HIR
test to its pre-slice coverage.

The attempted acceptable route is blocked inside the general expression
parser, outside this packet's owned implementation files. The existing
`parse_assign_expr` rematerialization path does not currently produce a
complete structured carrier for captured tokens such as
`Outer<T>::Count::value + 1`: the normal expression parser materializes only
the template owner/member prefix (`Outer_T_int_N_7::Count` in the concrete
probe) and does not preserve the remaining nested static member access as
`NK_VAR`/expression metadata. Reconstructing that carrier acceptably requires a
generic expression-parser repair for chained `Template<Args>::Nested::member`
scope handling, not another `parse_base_type()` token-shape fallback.

## Suggested Next

Continue Step 4 with a packet that owns the general expression parser path
needed by captured NTTP expression rematerialization, likely
`src/frontend/parser/impl/expressions.cpp` plus the existing focused test. The
repair should make `parse_assign_expr` produce a structured carrier for
`Template<Args>::Nested::member` expressions generically before any HIR
consumer work is considered.

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
- The removed HIR nested-owner evaluator was mostly carrier-driven but was not
  kept in this idea-139 slice because the parser/Sema producer is still
  blocked. If the generic expression-parser repair later proves HIR still needs
  a nested-owner consumer, either justify that as a consumer of the completed
  handoff in `todo.md` or move it to idea 140/a narrower HIR metadata blocker.
- The `cpp_positive_sema_template_nttp_default_runtime_cpp` proof case covers
  the default-NTTP side of this path: static-member value expressions must carry
  enclosing type bindings when evaluating defaults such as
  `is_void<T>::value || is_void<T>::value`.
- Current blocker: within the owned files, `base.cpp` can invoke
  `parse_assign_expr` over `captured_expr_tokens`, but it cannot make the
  expression parser consume and preserve chained scope after a template-id.
  Keeping a local token parser in `base.cpp` was rejected as testcase-shaped
  overfit, so no speculative implementation remains.

## Proof

Ran the delegated proof command after removing the rejected implementation:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp|cpp_hir_template_struct_arg_materialization|cpp_hir_template_value_arg_static_member_trait)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `888/888` matched tests.
Final proof log path: `test_after.log`.
