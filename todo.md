Status: Active
Source Idea Path: ideas/open/209_route4_publication_source_semantic_reader.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Route/Prepared Agreement for the Named Reader

# Current Packet

## Just Finished

Step 2 added Route 4/prepared agreement for only
`find_prepared_indirect_callee_source_producer(...)` in
`src/backend/mir/aarch64/codegen/calls.cpp`.

The named reader now validates current-block Route 4 publication evidence
against the prepared source producer record before using Route 4 identity:
prepared block/current BIR block relationship, result spelling and optional
value-name id, value type, source producer kind, source producer instruction
pointer and index, produced-value identity, and `before_instruction_index`.
When agreement succeeds, the indirect-callee reader returns the indexed BIR
instruction/result identity instead of reusing the prepared producer pointer, so
the Route 4 path is behavior-bearing. Missing, stale, wrong-reference, or
mismatched Route 4 evidence still falls back to the prepared producer identity
and leaves materialization/output policy prepared-owned.

Added focused dispatch coverage in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` with a stale
prepared select payload decoy: agreeing Route 4/prepared evidence must use the
indexed BIR select identity, while wrong-reference evidence preserves prepared
fallback selected-callee output.

## Suggested Next

Step 3 should stay narrow around any remaining requested Route 4 publication
reader negative coverage, especially duplicate/conflict or mismatch cases if the
supervisor wants them for this named reader. Do not broaden into call-boundary
readers, wrapper policy, printer/debug output, prepared API migration, or
expected-string weakening.

## Watchouts

Do not migrate or rename `prepared_call_boundary_source_value(...)`; it remains
an adjacent Route 4 reader, not part of this selected-reader slice. The previous
route-only/no-prepared assertion was removed because this fixture can still
materialize selected-callee output through existing Route 3 identity; do not use
that as Route 4 proof. Do not move indirect-callee materialization, scratch
selection, ABI register policy, call records, wrappers, printer/debug rows,
expected strings, or prepared API surfaces into Route 4.

## Proof

Ran the delegated proof exactly:
`cmake --build --preset default --target backend_aarch64_instruction_dispatch_test backend_prepared_lookup_helper_test && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$' > test_after.log`

Result: passed, 2/2 focused tests green. Canonical proof log:
`test_after.log`.
