# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 repaired one retained `$expr:` template-argument compatibility route in
`src/frontend/parser/impl/types/types_helpers.hpp`. Template instantiation
argument keys now prefer the structured parsed NTTP expression carrier
(`TemplateArgParseResult::expr` or `TypeSpec::array_size_expr`) over the
compatibility `$expr:` spelling when the carrier is present.

Focused parser coverage in `tests/frontend/frontend_parser_tests.cpp` now proves
two value arguments with the same structured expression carrier but different
stale `$expr:` strings produce the same instantiation key, and that the key no
longer retains `$expr:` text on the structured route.

## Suggested Next

Continue Step 4 by inspecting the remaining no-carrier template-argument
compatibility consumers. Prefer a route where parser/Sema already has
`template_arg_nttp_text_ids`, `captured_expr_tokens`, `expr`, or another direct
domain carrier at the consumer before removing rendered fallback behavior.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- `$expr:` text is still produced for compatibility/display and remains the
  no-carrier fallback in `make_template_instantiation_argument_key` when no
  parsed expression or token carrier exists.
- The no-carrier template-argument compatibility routes remain separate Step 4
  watch items. Do not count this packet as source-idea closure.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
