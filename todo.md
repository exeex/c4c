# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 investigated the direct nested template-origin pending-arg handoff in
`Parser::parse_base_type`. The exact rendered/display route examined is the
direct pending-template branch that enters `if (has_unresolved_type_arg ||
has_deferred_nttp_arg || has_incomplete_nttp_default)`, builds `arg_refs` for a
nested pending template through `ats.tpl_struct_origin` plus
`template_arg_refs_text(ats)`, and then stores those rendered refs through
`set_template_arg_debug_refs_text(&ts, arg_refs)` because
`parsed_args_have_simple_structured_carrier(concrete_args)` rejects nested
`tpl_struct_origin` / `tpl_struct_args` carriers.

An exploratory parser-only repair that switched nested origin-key/arg-carrier
pending args to typed `TemplateArgRef` storage was not kept. It passed the
focused `frontend_parser_lookup_authority_tests` check, but the delegated proof
then failed existing direct nested-template positive cases at the HIR
owner-struct pending boundary: `cpp_positive_sema_template_struct_advanced_cpp`
and `cpp_positive_sema_template_struct_nested_cpp` reported `delegated to owner
struct work` followed by `owner struct still pending` for function return,
function parameter, local declaration, member owner, member-owner AST, and
operator-call contexts. No parser implementation or test changes are left dirty.

## Suggested Next

Treat direct nested template-origin pending-arg carrier promotion as blocked on
HIR owner-struct pending resolution, not as a parser-only Step 4 slice. The next
coherent packet should either route to a HIR-owned owner-struct pending carrier
fix/split, or select a different parser/Sema metadata handoff that does not
cross the pending-template owner boundary.

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

Ran the delegated proof command during the blocked repair attempt:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest failed `2/887` matched tests:
`cpp_positive_sema_template_struct_advanced_cpp` and
`cpp_positive_sema_template_struct_nested_cpp`. Both failures are HIR
owner-struct pending blockers caused by the exploratory direct nested structured
carrier handoff. Final proof log path: `test_after.log`. The exploratory
implementation and test edits were reverted after capturing the blocker.
