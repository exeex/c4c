Status: Active
Source Idea Path: ideas/open/242_phase_e5_route6_scalar_call_use_source_identity_row_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Validate Target-Policy No-Change Surfaces

# Current Packet

## Just Finished

Step 4 validated that the selected Route 6 call-result source adapter did not
move call policy or output ownership into Route 6.

Files inspected:
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_test.cpp`
- `tests/backend/bir/CMakeLists.txt`
- `tests/backend/mir/CMakeLists.txt`
- `plan.md`
- `todo.md`

The selected adapter remains limited to
`call_result_source_register_route6_evidence(...)` and
`record_call_result_source_register(...)`. Route 6 only decides
`Agreed`/`Fallback` for semantic result source identity after matching the
prepared destination value name; register spelling, register bank,
`CallAbi` role, prepared value id/name, occupied-register set, result lanes,
stack-home stores, retargeting, and final publication still come from
`PreparedCallResultPlan`, prepared value-home lookups, prepared after-call lane
bindings, and AArch64 lowering.

No expectation, baseline, helper rename, unsupported downgrade, facade-only
move, broad-wrapper claim, named-case shortcut, or wrapper/output relabeling was
found. No ownership movement was found for ABI placement, frame/register policy,
call-wrapper policy, helper/carrier protocol, result lanes, outgoing stack
layout, formatting, instruction selection, emission policy, or wrapper output.
`backend_aarch64_call_boundary_owner` continues to assert that every selected
Route 6 evidence case preserves prepared register publication, while
`backend_prepared_lookup_helper`, `backend_prepare_frame_stack_call_contract`,
and the x86 handoff boundary registration remain nearby public compatibility
proof only.

## Suggested Next

Execute Step 5 from `plan.md`: prepare closure evidence for the selected Route
6 call-result source adapter without broadening the source idea.

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_prepared_lookup_helper|backend_prepare_frame_stack_call_contract|backend_x86_handoff_boundary)$' --output-on-failure > test_after.log
```

## Watchouts

- Keep the adapter boundary to the selected call-result source register
  evidence read only.
- Do not treat idea 238 as proof beyond x86 Route 6 scalar `i32` route-debug /
  `ConsumedPlans` compatibility.
- Step 4 did not claim broad x86 call-wrapper migration, route-wide x86
  readiness, riscv readiness, cross-target wrapper convergence, whole
  `call_plans`, `PreparedFunctionLookups`, `PreparedBirModule`, or draft 155
  retirement readiness.
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
  `backend_x86_handoff_boundary` remains in the delegated regex but was not
  matched by the current CTest subset, consistent with the 3/3 supervisor
  baseline stated for this packet.

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
