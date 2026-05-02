# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 narrowed a remaining Sema no-carrier template-argument compatibility
consumer in `bind_consteval_call_env`. When `Node::template_arg_exprs` carries
the NTTP expression but evaluation misses, Sema now treats that structured
carrier as authoritative and does not fall through to stale rendered
`template_arg_nttp_names` lookup.

Focused authority coverage in
`tests/frontend/frontend_parser_lookup_authority_tests.cpp` now builds a
callee with an unevaluable structured NTTP expression plus stale `$expr:`
fallback text and proves no consteval NTTP binding is produced from the
rendered name.

## Suggested Next

Continue Step 4 by inspecting remaining parser/Sema no-carrier
template-argument compatibility consumers. Prefer routes where
`template_arg_nttp_text_ids`, `captured_expr_tokens`, `expr`,
`TypeSpec::array_size_expr`, or another direct domain carrier is already
present before removing rendered fallback behavior.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- `$expr:` text is still produced for compatibility/display and remains a
  no-carrier fallback only where no parsed expression, token, TextId, or other
  structured carrier exists at that consumer.
- This packet did not alter `make_template_instantiation_argument_key`; its
  no-carrier `$expr:` compatibility route remains a separate watch item.
- The no-carrier template-argument compatibility routes remain separate Step 4
  watch items. Do not count this packet as source-idea closure.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
