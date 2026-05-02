# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 re-inventoried the remaining parser/Sema metadata handoff routes and
selected the smallest parser-owned route still restoring semantic identity from
rendered spelling: the qualified typedef function-style cast probe in
`parse_primary`, where `last_resolved_typedef` could be restored from
`saved_typedef_fallback` even when no `TextId` metadata existed.

The repair makes `last_resolved_typedef` a TextId-only semantic carrier.
`Parser::set_last_resolved_typedef(kInvalidText, ...)` now clears the carrier
instead of retaining rendered spelling, and the qualified typedef cast restore
path no longer rehydrates a rendered-only saved typedef name. Focused parser
authority coverage now seeds a rendered-only stale typedef state, parses a
qualified structured typedef cast, verifies the structured typedef payload is
used, and verifies the stale rendered typedef spelling is not restored.

## Suggested Next

Continue Step 4 with another parser/Sema-owned handoff inventory pass and pick
one remaining route with both producer and consumer in `src/frontend/parser` or
`src/frontend/sema`. A good next candidate is the template-parameter helper
route in `src/frontend/parser/impl/types/types_helpers.hpp`, where
`find_template_param_index` still accepts a fallback spelling and can intern it
when the caller lacks a `TextId`; repair that only if the callers can pass
structured parameter metadata without widening into HIR.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- Rendered typedef spelling is still allowed for display and diagnostics, but
  `last_resolved_typedef` should not be used as semantic authority unless its
  `TextId` is present.
- The retained `$expr:` and no-carrier template-argument compatibility routes
  remain separate Step 4 watch items. Do not count this packet as source-idea
  closure.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
