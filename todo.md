Status: Active
Source Idea Path: ideas/open/153_bir_select_chain_direct_global_identity.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Acceptance Review And Close Payload

# Current Packet

## Just Finished

Step 6 of `plan.md` is complete. Recorded close-ready evidence for the BIR
select-chain direct-global identity runbook.

- BIR vocabulary now includes `mir::BirSelectChainIdentityRequest`,
  `mir::BirSelectChainDirectGlobalDependency`, and
  `mir::BirSelectChainIdentity`.
- BIR query APIs now cover `mir::find_bir_select_chain_source_producer(...)`,
  `mir::find_bir_select_chain_direct_global_dependency(...)`,
  `mir::find_bir_select_chain_scalar_materialization_eligibility(...)`, and
  `mir::find_bir_select_chain_identity(...)`.
- Equivalence proof in `backend_prepared_lookup_helper_test` compares prepared
  and BIR answers for select root, direct global root, local/no-dependency root,
  nested non-select direct-global root, before-boundary rejection, root type or
  name mismatch, and missing-root fail-closed paths.
- Step 5 switched only the route-local
  `select_chain_contains_direct_global_load(...)` identity read in
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` to
  `mir::find_bir_select_chain_direct_global_dependency(...)`.
- Prepared select-chain queries remain available as oracle/fallback authority:
  `find_prepared_direct_global_select_chain_dependency(...)` and
  `find_prepared_scalar_select_chain_materialization(...)` still serve the
  unswitched materialization, prealloc, and call-routing consumers.
- Rejected target-specific state did not enter the BIR relationship: no target
  materialization cost, register availability, publication routing, call ABI
  behavior, or final AArch64 move/branch choice is represented in the BIR
  select-chain identity records or query request.

## Suggested Next

Supervisor can hand this packet to lifecycle closure review for the active
`ideas/open/153_bir_select_chain_direct_global_identity.md` runbook.

## Watchouts

- This was a todo-only closure evidence packet; no code or log file changed.
- `test_before.log` is the accepted Step 5 proof log for the final
  code-changing scope.
- `review/phase_a_steps_1_4_route_review.md` remains a transient preexisting
  review artifact and was not touched.

## Proof

No new proof was required for this todo-only closure packet. Accepted Step 5
matched regression proof command:

```bash
set -o pipefail && cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure | tee test_after.log
```

Result: passed, `2/2` tests green:

- `backend_prepared_lookup_helper`
- `backend_aarch64_instruction_dispatch`

Accepted proof log path: `test_before.log`.
