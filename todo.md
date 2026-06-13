Status: Active
Source Idea Path: ideas/open/242_phase_e5_route6_scalar_call_use_source_identity_row_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Implement The Agreement-Gated Adapter

# Current Packet

## Just Finished

Step 2 implemented the smallest local agreement-gated adapter around the
selected AArch64 call-result source register Route 6 evidence reader.
`call_result_source_register_route6_evidence(...)` now accepts
`bir::route6_find_call_result_source(...)` only when the returned Route 6
result identity agrees with the prepared destination value name from the
prepared value home.

The new local helper
`route6_call_result_source_agrees_with_prepared(...)` first honors a Route 6
`result_identity.name_id` when present, then falls back to resolving the Route
6 result value through the existing prepared name table. Missing, invalid,
duplicate, unavailable, prepared-mismatch, Route 6 identity-mismatch, or
non-agreement paths continue to report
`CallResultSourceRegisterRoute6Evidence::Fallback`.

The scalar-state publication path remains prepared-owned:
`record_call_result_source_register(...)` still derives register spelling,
bank, placement conversion, contiguous width, occupied registers,
destination value id, and `CallAbi` publication from
`PreparedCallResultPlan` plus prepared value-home lookups. Route 6 does not
own ABI placement, frame/register policy, wrapper policy, helper/carrier
protocol, result lanes, outgoing stack layout, formatting, instruction
selection, emission policy, or wrapper output.

Focused coverage added one nearby same-feature fallback case in
`backend_aarch64_call_boundary_owner`: a Route 6 result record with a
conflicting prepared value-name identity falls back while preserving the
prepared register publication.

## Suggested Next

Execute Step 3 from `plan.md`: prove fallback and public compatibility for the
selected Route 6 call-result source adapter, reusing the same focused proof
subset unless the supervisor chooses otherwise.

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
