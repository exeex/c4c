Status: Active
Source Idea Path: ideas/open/242_phase_e5_route6_scalar_call_use_source_identity_row_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Fallback And Public Compatibility

# Current Packet

## Just Finished

Step 3 proved fallback and public compatibility for the selected Route 6
call-result source adapter using existing Step 2 coverage; no additional tests
were needed.

`backend_aarch64_call_boundary_owner` now covers the selected positive
agreement row and explicit fallback/non-agreement rows for null Route 6 index,
missing Route 6 fact, invalid call boundary, duplicate/conflicting Route 6
fact, prepared destination mismatch, and Route 6 identity mismatch. Every row
also asserts that prepared register publication is preserved: register spelling,
bank, `CallAbi` role, prepared value id/name, and occupied-register set still
come from `PreparedCallResultPlan` plus prepared value-home lookups.

Public compatibility remains covered by `backend_prepared_lookup_helper` and
`backend_prepare_frame_stack_call_contract`: the prepared/BIR call-result
source identity helper continues to expose only destination value identity,
Route 6 call-result records expose provenance without ABI placement, result
lane identity remains nearby same-feature coverage only, and prepared
late-publication/result-plan helpers remain authoritative for call-plan and
storage facts.

Idea 238 remains narrow x86 Route 6 scalar `i32` route-debug /
`ConsumedPlans` evidence only. This Step 3 proof does not claim broad x86,
riscv, aggregate, wrapper, whole `call_plans`, `PreparedFunctionLookups`,
`PreparedBirModule`, or draft 155 retirement readiness.

## Suggested Next

Execute Step 4 from `plan.md`: perform the route-quality and no-drift audit for
the selected Route 6 call-result source adapter.

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_prepared_lookup_helper|backend_prepare_frame_stack_call_contract|backend_x86_handoff_boundary)$' --output-on-failure > test_after.log
```

## Watchouts

- Keep the adapter boundary to the selected call-result source register
  evidence read only.
- Do not treat idea 238 as proof beyond x86 Route 6 scalar `i32` route-debug /
  `ConsumedPlans` compatibility.
- Do not delete, privatize, rename, or hide whole `call_plans`,
  `PreparedFunctionLookups`, or `PreparedBirModule`.
- Do not move ABI placement, frame/register policy, call-wrapper policy,
  helper/carrier protocol, result lanes, outgoing stack layout, formatting,
  instruction selection, emission policy, or wrapper output into Route 6
  ownership.
- Do not rewrite expectations, helper names, supported-path status, wrapper
  output, or baselines as proof.
- Nearby same-feature cases to preserve in proof: matching/null/missing/
  invalid-boundary/duplicate/prepared-mismatch/Route 6 identity-mismatch
  result-source evidence in `backend_aarch64_call_boundary_owner`; BIR
  call-result source and result-lane source identity/index status rows in
  `backend_prepared_lookup_helper`;
  prepared late-publication/result-plan contracts in
  `backend_prepare_frame_stack_call_contract`; and the narrow x86
  `ConsumedPlans` scalar call-argument compatibility row in
  `backend_x86_handoff_boundary`.
- Result-lane identity is nearby same-feature coverage, not part of the
  selected implementation row unless the supervisor opens a separate packet.
- The exact delegated CTest regex currently matched the same three tests as
  the supervisor baseline: `backend_aarch64_call_boundary_owner`,
  `backend_prepare_frame_stack_call_contract`, and
  `backend_prepared_lookup_helper`.

## Proof

Ran the exact supervisor-selected proof command:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_prepared_lookup_helper|backend_prepare_frame_stack_call_contract|backend_x86_handoff_boundary)$' --output-on-failure > test_after.log
```

Result: 3/3 passed.

Tests matched:
- `backend_aarch64_call_boundary_owner`
- `backend_prepare_frame_stack_call_contract`
- `backend_prepared_lookup_helper`

Proof log path: `test_after.log`.
