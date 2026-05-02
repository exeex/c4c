# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 tightened the consteval type-binding route in
`resolve_type(TypeSpec, ConstEvalEnv)`. The exact route examined was a
`TB_TYPEDEF` `TypeSpec` with intrinsic `tag_text_id` or template-param owner
metadata plus a stale rendered `TypeSpec::tag`: after intrinsic key/text probes
missed, `resolve_type()` could still rediscover structured/TextId metadata by
looking up the rendered tag, and could then fall through to flat
`type_bindings`.

`resolve_type()` now checks intrinsic TypeSpec key/text carriers before any
rendered-name metadata lookup. When a structured/TextId type-binding channel is
present and the intrinsic carrier does not resolve, it returns the original
`TypeSpec` instead of reopening rendered `TypeSpec::tag` lookup. Legacy flat
`type_bindings` remains available for flat-only/no-carrier environments so
existing consteval callers that do not yet supply structured maps continue to
work. Added focused drift coverage through `bind_consteval_call_env()` proving
the legacy no-carrier route still works while `tag_text_id` and structured
template-param carriers suppress stale rendered type-binding re-entry.

## Suggested Next

Continue Step 4 by selecting another parser/Sema metadata handoff that remains
inside parser/Sema ownership, with special attention to consteval call paths
that still pass only flat type bindings and no structured/TextId binding
channels.

## Watchouts

- The source idea remains active; Step 4 progress is not source-idea closure.
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

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
