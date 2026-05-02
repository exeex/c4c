# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 repaired the remaining `substitute_record_carrier_arg` NTTP
parameter-index path in `src/frontend/parser/impl/types/base.cpp`. Value
carrier substitution now uses a carried `nttp_text_id` to select the template
parameter slot and only falls back to rendered `nttp_name` spelling when no
TextId carrier exists.

Focused authority coverage in
`tests/frontend/frontend_parser_lookup_authority_tests.cpp` now drives the
record-member typedef sidecar path with a concrete wrapper carrying `N=3` and
`M=9`, then drifts the sidecar's rendered NTTP name to `M`; resolution still
selects the `N`/`Owner<3>` member typedef through the carried TextId.

## Suggested Next

Continue Step 4 by reviewing whether any remaining parser/Sema
template-argument compatibility consumers still choose semantic slots from
rendered names when a structured TextId, expression, token, or TypeSpec carrier
is already present.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
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
