# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 repaired the parser-owned template-parameter pattern lookup route in
`src/frontend/parser/impl/types/types_helpers.hpp`. `find_template_param_index`
no longer accepts or interns a fallback spelling, and its type/value callers now
bind only from structured `TextId` carriers: `TypeSpec::template_param_text_id`
or `TypeSpec::tag_text_id` for type patterns, and
`template_arg_nttp_text_ids` for value patterns.

Focused lookup-authority coverage now proves stale rendered type and NTTP
pattern names still bind when the structured carriers are present, and do not
recover through rendered spelling when those carriers are absent. The existing
specialization TextId test was updated to provide the structured type-pattern
carrier explicitly.

## Suggested Next

Continue Step 4 with another parser/Sema-owned handoff inventory pass and pick
one remaining route with both producer and consumer in `src/frontend/parser` or
`src/frontend/sema`. Prefer one of the retained `$expr:` or no-carrier
template-argument compatibility routes only if the parser/Sema producer can
publish structured metadata without widening into HIR.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- Rendered typedef spelling is still allowed for display and diagnostics, but
  `last_resolved_typedef` should not be used as semantic authority unless its
  `TextId` is present.
- Rendered template parameter spellings are still retained for compatibility
  output and debug/display text, but `find_template_param_index` is no longer a
  spelling-to-identity recovery path.
- The retained `$expr:` and no-carrier template-argument compatibility routes
  remain separate Step 4 watch items. Do not count this packet as source-idea
  closure.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
