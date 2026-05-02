# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 repaired a remaining parser-side no-carrier handoff route where
`Node::template_arg_exprs` was already available but local `ParsedTemplateArg`
reconstruction dropped it before member-typedef and template-instantiation
lookup. `parsed_template_arg_from_node_slot` now carries the structured NTTP
expression node along with value/name/TextId metadata, and the inspected
parser consumers use that helper instead of rebuilding partial no-expr args.

Focused authority coverage in
`tests/frontend/frontend_parser_lookup_authority_tests.cpp` now proves a
reconstructed value argument with stale `$expr:` display text keeps the parsed
expression carrier, omits `$expr:` from the canonical key, and remains stable
when the stale rendered text changes.

## Suggested Next

Continue Step 4 by inspecting the remaining no-carrier template-argument
compatibility consumers. Prefer another route where parser/Sema already has
`template_arg_nttp_text_ids`, `template_arg_exprs`, `captured_expr_tokens`,
`expr`, or another direct domain carrier at the consumer before removing
rendered fallback behavior.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- `$expr:` text is still produced for compatibility/display and remains the
  no-carrier fallback in `make_template_instantiation_argument_key` when no
  parsed expression or token carrier exists.
- `parsed_template_arg_from_node_slot` intentionally does not synthesize a
  structured carrier when the AST node lacks `template_arg_exprs`; no-carrier
  compatibility behavior remains limited to those truly carrierless cases.
- The no-carrier template-argument compatibility routes remain separate Step 4
  watch items. Do not count this packet as source-idea closure.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
