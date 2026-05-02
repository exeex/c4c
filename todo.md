# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 repaired the parser type-handling handoff for origin-key-only nested
template arguments in the alias/template-arg route. A type `TemplateArgRef`
with `tpl_struct_origin_key` or nested `tpl_struct_args` is no longer treated
as an unstructured debug-text fallback, and template-arg rendering now prefers
the structured template-origin key over stale `tpl_struct_origin` or
`TemplateArgRef::debug_text` spelling.

The removed/gated fallback route is the alias/template-arg refresh path that
rebuilt follow-on `tag` text through rendered `template_arg_refs_text(...)`
while stale nested debug text was still present. Alias substitution now
normalizes origin-key carriers before refreshing display text, so a drifted
`@RenderedInner...` debug payload remains display-only while the structured
`Outer`/`Inner` origin keys and substituted structured `int` arg drive the
result.

## Suggested Next

Continue Step 4 with the next parser-to-Sema metadata handoff gap selected by
the supervisor. A coherent next packet would be direct nested template-origin
record materialization, but it should stay separate from this alias handoff
because direct nested pending-template args still feed existing HIR owner-struct
work.

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
- `captured_expr_tokens` is parser-local structured metadata. Do not promote
  stale `nttp_name`/`$expr:` text back to semantic authority when the token
  carrier exists.
- `tpl_struct_origin` remains compatibility/display spelling. Do not use it to
  rediscover template primaries or decide Sema `TypeSpec` equivalence when
  `tpl_struct_origin_key` or structured arg carriers exist.
- Simple type-only pending template args now use `TemplateArgRef` structure.
  The broad direct nested template-origin path still stays on the legacy
  display route to avoid HIR `owner struct still pending` regressions; this
  packet only tightened the alias/template-arg origin-key handoff so stale
  nested debug text is not semantic authority there.
- Do not reintroduce rendered qualified-text parsing, `$expr:` debug-text
  semantic authority, string/string_view semantic lookup parameters, fallback
  spelling, expectation downgrades, or named-test shortcuts.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully. CTest passed `887/887` matched tests,
including `frontend_parser_tests`,
`frontend_parser_lookup_authority_tests`,
`eastl_cpp_external_utility_frontend_basic_cpp`, and the
`cpp_positive_sema_.*` subset. Final proof log path: `test_after.log`.
