# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 narrowed the alias-template member-typedef carrier substitution paths in
`src/frontend/parser/impl/types/base.cpp`. `substitute_carrier_arg` and the
nested owner-alias substitution now select template parameter slots from
`ParsedTemplateArg::nttp_text_id` and `TypeSpec::tag_text_id` when present, only
falling back to rendered `nttp_name`/`tag` spelling when no structured carrier
exists.

Focused authority coverage in
`tests/frontend/frontend_parser_lookup_authority_tests.cpp` now drives
`Alias<3, 9>` through an alias-template member-typedef owner arg whose carried
TextId names `N` while stale rendered `nttp_name` says `M`; resolution still
selects the registered `Owner<3>::type` binding instead of `Owner<9>`.

## Suggested Next

Continue Step 4 by reviewing whether any remaining parser/Sema
template-argument compatibility consumers still choose semantic slots from
rendered names when a structured TextId, expression, token, or TypeSpec carrier
is already present; likely next candidates are the remaining `alias_param_ref_text_id`
and `owner_alias_param_ref_text_id` type-arg consumers that still need route-by-route
inspection for available carriers.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- The rendered-name branches in the edited substitution lambdas remain only as
  no-carrier compatibility paths for older payloads that lack `nttp_text_id` or
  `tag_text_id`.
- `$expr:` text is still produced for compatibility/display and remains a
  no-carrier fallback only where no parsed expression, token, TextId, or other
  structured carrier exists at that consumer.
- This packet did not alter `make_template_instantiation_argument_key`; its
  no-carrier `$expr:` compatibility route remains a separate watch item.
- The no-carrier template-argument compatibility routes remain separate Step 4
  watch items. Do not count this packet alone as source-idea closure.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
