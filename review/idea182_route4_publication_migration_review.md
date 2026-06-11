# Idea 182 Route 4 Publication Migration Review

## Review Scope

- Source idea: `ideas/closed/182_phase_e_route4_publication_view_consumer_migration.md`
- Chosen base commit: `bd9b1bab2^`
- Reviewed range: `bd9b1bab2^..9d0c57a71`
- Commit count in range: 7

I used `bd9b1bab2^` because `bd9b1bab2` is the activation commit for idea 182 (`[plan] Activate Phase E Route 4 publication migration plan`) and `9d0c57a71` is the closure commit. The intervening history is unambiguous: target selection, Route 4 boundary, Route 4 preference, equivalence proof, handoff, and lifecycle close.

## Findings

No blocking findings.

The implementation stays within idea 182 scope. The production diff adds a local Route 4 identity helper for `current_block_entry_publication_register(...)` and does not migrate additional consumers. The selected reader still begins from the prepared current-block entry publication query, then consults Route 4 identity only for the selected destination value boundary.

Prepared fallback and oracle behavior are preserved. `current_block_entry_publication_register(...)` still returns no publication when the prepared helper cannot produce an available publication or register spelling, and it keeps prepared destination id/name as the default. A valid Route 4 identity can replace the semantic destination id/name, but unavailable or invalid Route 4 data falls back to prepared identity rather than deleting or narrowing the prepared helper surface.

The tests are route-quality tests, not testcase-shape overfit. The AArch64 dispatch test constructs a BIR phi publication route, checks the Route 4 identity directly, verifies the selected reader prefers valid Route 4 identity, and separately covers Route 4 unavailable, wrong-type, and wrong-name fallback cases. The BIR helper test strengthens facade/direct-reference equivalence by comparing `route_status` in the existing fail-closed cases.

I did not find prepared API deletion, target-policy leakage into BIR, expectation downgrades, or named-testcase shortcuts. Register parsing and target register view handling remain in AArch64 codegen; Route 4/BIR supplies publication identity only.

## Evidence

- Idea scope requires one AArch64 current/block-entry publication reader and prepared fallback/oracle surfaces: `ideas/closed/182_phase_e_route4_publication_view_consumer_migration.md:14`.
- Idea explicitly excludes prepared helper deletion and BIR ownership of target policy: `ideas/closed/182_phase_e_route4_publication_view_consumer_migration.md:23`.
- Route 4 helper boundary is local to dispatch publication codegen: `src/backend/mir/aarch64/codegen/dispatch_publication.cpp:214`.
- Prepared helper remains the initial oracle/fallback query: `src/backend/mir/aarch64/codegen/dispatch_publication.cpp:253`.
- Valid Route 4 identity is preferred only after the prepared publication is available: `src/backend/mir/aarch64/codegen/dispatch_publication.cpp:272`.
- Prepared destination identity remains the fallback default: `src/backend/mir/aarch64/codegen/dispatch_publication.cpp:279`.
- Target register parsing and register view remain in AArch64 codegen: `src/backend/mir/aarch64/codegen/dispatch_publication.cpp:287`.
- Route 4 direct identity and selected-reader preference are tested: `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp:27419`.
- Route unavailable, wrong-type, and wrong-name fallback cases are tested: `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp:27448`.
- BIR facade/direct-reference route status equivalence is strengthened: `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:7266`.

## Validation

Recorded focused proof in `test_after.log`:

```bash
cmake --build build --target backend_aarch64_instruction_dispatch_test backend_prealloc_block_entry_publications_test backend_prepared_lookup_helper_test &&
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prealloc_block_entry_publications|backend_prepared_lookup_helper)$' --output-on-failure
```

Result: 3/3 tests passed.

For this one-consumer migration, the focused proof is sufficient for route quality. The rejected `test_baseline.new.log` should remain rejected as recorded by the closure note; it is not evidence that this route is overfit, but it also means this slice did not produce a new full-suite acceptance baseline.

## Judgments

- Idea alignment: `matches source idea`
- Runbook transcription: `plan matches idea`
- Route alignment: `on track`
- Technical debt: `acceptable`
- Validation sufficiency: `narrow proof sufficient`
- Reviewer recommendation: `continue current route`

No route reset or follow-up lifecycle repair is needed for idea 182.
