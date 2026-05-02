# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 tightened the remaining parser-owned `$expr:` / rendered-template-arg
route in `Parser::parse_base_type`. The exact routes examined were the direct
template prelim evaluator branch that previously re-lexed `ParsedTemplateArg`
`nttp_name` when it began with `$expr:`, plus the rendered pending-template
helpers `render_template_arg_ref()` and `template_arg_refs_text()` that could
preserve `$expr:` debug spelling for value args even when `ParsedTemplateArg`
`expr`, `captured_expr_tokens`, `nttp_text_id`, or `TemplateArgRef` value
`TypeSpec::array_size_expr` carriers existed.

The evaluator-side `$expr:` re-lex branch is now gated by
`parsed_nttp_arg_has_structured_carrier()`, and the rendered helpers now avoid
debug-text fallback when a value arg has a structured parser carrier. Added
same-feature drift coverage for pending direct value args with parsed-expression
and `nttp_text_id` carriers, plus nested pending rendered args that previously
could preserve `$expr:` text after a `TypeSpec::array_size_expr` carrier was
available.

## Suggested Next

Continue Step 4 by selecting another parser/Sema metadata handoff that remains
inside parser/Sema ownership, or route direct nested template-origin carrier
promotion separately through the known HIR owner-struct pending blocker.

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
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `887/887` matched tests.
Final proof log path: `test_after.log`.
