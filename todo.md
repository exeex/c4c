# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 narrowed the dependent template-specialization value-argument consumer
in `src/frontend/parser/ast.hpp`. `is_dependent_template_struct_specialization`
now treats `Node::template_arg_nttp_text_ids` as authoritative when present and
only consults rendered `template_arg_nttp_names` as the no-carrier compatibility
path.

Focused authority coverage in
`tests/frontend/frontend_parser_lookup_authority_tests.cpp` now drives a
template value arg whose structured TextId names `N` while stale rendered text
says `RenderedDrift`, verifies the specialization is still dependent, verifies a
non-matching TextId blocks rendered-name recovery, and preserves rendered-name
compatibility when no TextId carrier exists.

## Suggested Next

Continue Step 4 by reviewing the remaining no-carrier template-argument
compatibility routes, especially `make_template_instantiation_argument_key` and
any `$expr:` display-text consumers, for places where parsed expression or
TextId carriers should become authoritative before rendered strings. The
specific `is_dependent_template_struct_specialization` value-arg route is now
narrowed to TextId-first/no-carrier-name fallback.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- `Node::template_arg_nttp_names` remains a compatibility path in the edited
  dependency check only when the parallel `template_arg_nttp_text_ids` slot is
  absent or invalid.
- `$expr:` text is still produced for compatibility/display and remains a
  no-carrier fallback only where no parsed expression, token, TextId, or other
  structured carrier exists at that consumer.
- This packet inspected `make_template_instantiation_argument_key`; its value
  route already prefers parsed expression and captured-token carriers before the
  `$expr:` no-carrier fallback, so it was left unchanged.
- The no-carrier template-argument compatibility routes remain separate Step 4
  watch items. Do not count this packet alone as source-idea closure.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
