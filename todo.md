# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 narrowed the remaining alias-template type-parameter consumers in
`src/frontend/parser/impl/types/base.cpp`. The dependent owner-arg detector and
the direct alias typedef substitution fallback now select alias parameter slots
from `TypeSpec::tag_text_id` before consulting rendered `tag` spelling; rendered
lookup remains only as the no-carrier compatibility path.

Focused authority coverage in
`tests/frontend/frontend_parser_lookup_authority_tests.cpp` now drives
`Alias<int>` through an aliased `TB_TYPEDEF` whose structured TextId names `T`
while stale rendered `tag` says `RenderedDrift`; resolution still substitutes
the actual `int` argument.

## Suggested Next

Continue Step 4 by reviewing the remaining no-carrier template-argument
compatibility routes, especially `make_template_instantiation_argument_key` and
any `$expr:` display-text consumers, for places where parsed expression or
TextId carriers should become authoritative before rendered strings.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- The rendered-name branches in the edited substitution lambdas remain only as
  no-carrier compatibility paths for older payloads that lack `nttp_text_id` or
  `tag_text_id`.
- Remaining `alias_param_ref_text_id` and `owner_alias_param_ref_text_id` calls
  in `base.cpp` are now either value-name fallbacks after `nttp_text_id` misses
  or type-name fallbacks after `TypeSpec::tag_text_id` misses.
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
