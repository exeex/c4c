Status: Active
Source Idea Path: ideas/open/157_deferred_syntax_text_payload_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Audit Remaining Text Payloads And Compatibility Bridges

# Current Packet

## Just Finished

Completed Step 5 audit for the deferred text payloads and compatibility
bridges in the touched parser and HIR surfaces. Retained text payloads are:
`$expr:` NTTP names from `capture_template_arg_expr` and
`try_parse_template_non_type_expr` as deferred syntax/debug mirrors;
`TemplateArgRef::debug_text` emitted by parser/HIR materialization as
debug/display spelling or no-metadata compatibility; `template_arg_nttp_names`
as a compatibility mirror beside `template_arg_nttp_text_ids`;
`deferred_member_type_name` as a syntax/display mirror beside
`deferred_member_type_text_id` and owner metadata; string `display_key` and
encoded type refs as final display/mangling or string-only compatibility input;
and literal bool/integer text as literal payload.

No additional code demotion was needed in this packet. The audited lookup
surfaces already require structured carriers before text fallback: forwarded
NTTP `TextId` misses return/continue instead of reopening rendered-name lookup;
same-owner typed type-param carriers block `debug_text` scalar and pack lookup
after a miss; structured deferred member lookup uses owner/text metadata before
legacy owner spelling; and text-based string materialization remains bounded to
no-typed-carrier compatibility. The retained foreign-owner type-pack bridge is
the only structured-plus-text bridge kept, for nested owner handoff where the
current primary cannot resolve another template owner's pack carrier directly.
Its removal condition is a structured cross-owner pack binding key that can map
foreign owner/index carriers without using rendered names.

## Suggested Next

Proceed to Step 6 validation and closure-readiness review, including broader
supervisor-selected validation if this route is being treated as a milestone.

## Watchouts

- `TemplateArgRef::debug_text` and `template_arg_nttp_names` should remain
  display/debug/no-carrier compatibility only. New semantic paths should prefer
  `TextId`, owner/index, record, qualified-name, or expression carriers first.
- The foreign-owner type-pack bridge remains compatibility debt, but it is
  bounded to foreign-owner/index carriers and should not be broadened to
  same-owner structured misses.
- No tests were added because this Step 5 packet found no behavior change to
  prove beyond the existing stale-payload coverage.

## Proof

Ran the delegated proof:

`cmake --build --preset default && ctest --test-dir build -R '(frontend_parser_lookup_authority|frontend_hir_lookup_tests|cpp_hir_.*template|frontend_hir_.*template|hir_case|cpp_positive_sema_variadic_template_arg_sizeof_pack_parse_cpp)' --output-on-failure > test_after.log 2>&1`

Result: passed. `test_after.log` reports 48/48 tests passed.
