# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 narrowed the Sema consteval forwarded-NTTP handoff in
`src/frontend/sema/consteval.cpp`. `bind_consteval_call_env` now treats a
present forwarded `template_arg_nttp_text_ids` slot as authoritative even when
the outer consteval environment has no TextId/key map available, so it no longer
reopens legacy rendered `template_arg_nttp_names` lookup in that carrier-present
case.

Focused authority coverage in
`tests/frontend/frontend_parser_lookup_authority_tests.cpp` now extends
`test_consteval_forwarded_nttp_uses_text_id_not_rendered_name` with a no-map
outer environment whose legacy rendered name would otherwise bind. The test
verifies the TextId carrier blocks that rendered-name recovery.

## Suggested Next

Continue Step 4 review after the next review trigger. The remaining rendered
template-argument routes to scrutinize are no-carrier fallbacks only; verify any
next route has no parsed expression, token, TextId, record-def, or origin-key
carrier before preserving rendered-string compatibility.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- `Node::template_arg_nttp_names` remains a compatibility path in
  `bind_consteval_call_env` only when there is no parsed expression carrier and
  no forwarded TextId carrier for the template argument.
- `$expr:` text is still produced for compatibility/display and remains a
  no-carrier fallback only where no parsed expression, token, TextId, or other
  structured carrier exists at that consumer.
- The no-carrier template-argument compatibility routes remain separate Step 4
  watch items. Do not count this packet alone as source-idea closure.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
