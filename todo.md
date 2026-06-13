Status: Active
Source Idea Path: ideas/open/242_phase_e5_route6_scalar_call_use_source_identity_row_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Select The Route 6 Scalar Call-Use Source Reader

# Current Packet

## Just Finished

Step 1 selected exactly one Route 6 scalar call-use source reader for the
agreement-gated adapter: the AArch64 call-result source register evidence read
through `call_result_source_register_route6_evidence(...)`, reached from
`record_call_result_source_register(...)` in
`src/backend/mir/aarch64/codegen/calls.cpp`.

Selected Route 6 fact:
`bir::route6_find_call_result_source(...)` over the
`Route6CallUseSourceIndex::result_records` row for the current BIR call
boundary. The selected row is the call result role
`Route6CallUseValueRole::Result`, keyed by block, call instruction index,
callee, and the BIR result value.

Current prepared call-plan answer:
`prepare::find_prepared_call_result_late_publication(*call_plan.result)` plus
the indexed prepared value home for `result.destination_value_id`. The prepared
answer remains authoritative for source register name/bank/placement,
contiguous width, occupied registers, destination value id, and the emitted
`CallAbi` register publication recorded in `BlockScalarLoweringState`.

Fallback path:
`call_result_source_register_route6_evidence(...)` reports
`CallResultSourceRegisterRoute6Evidence::Fallback` and
`record_call_result_source_register(...)` preserves the existing prepared
publication path when the Route 6 index is absent, empty, invalid for the
current block/instruction/callee, missing the result fact, non-unique,
mismatched against the prepared result value/home, unavailable, or not
`Route6CallUseStatus::Available`. The selected reader does not own ABI
placement, frame/register policy, wrapper policy, helper/carrier protocol,
result lanes, outgoing stack layout, formatting, instruction selection,
emission policy, or wrapper output.

Why this is semantic source identity only:
the Route 6 read only checks that the BIR call result value identity agrees
with the prepared destination value identity. Register spelling, bank,
placement conversion, occupied register set, and scalar-state publication are
still derived from `PreparedCallResultPlan` and prepared value-home lookups.

Source inspection used:
- `c4c-clang-tool-ccdb function-signatures src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees src/backend/mir/aarch64/codegen/calls.cpp call_result_source_register_route6_evidence build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callers src/backend/mir/aarch64/codegen/calls.cpp call_result_source_register_route6_evidence build/compile_commands.json`
- `rg`/`sed` inspection of `calls.cpp`, `calls.hpp`, `dispatch.cpp`,
  `src/backend/bir/bir.cpp`, `src/backend/prealloc/calls.hpp`, and focused
  backend tests.

## Suggested Next

Execute Step 2 from `plan.md`: implement the smallest local agreement-gated
adapter around the selected AArch64 call-result source register evidence read,
accepting the Route 6 result-source fact only when it agrees with the prepared
call-result publication answer and preserving prepared fallback otherwise.

Recommended Step 2/3 proof subset:

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
  invalid-boundary/duplicate/prepared-mismatch result-source evidence in
  `backend_aarch64_call_boundary_owner`; BIR call-result source and result-lane
  source identity/index status rows in `backend_prepared_lookup_helper`;
  prepared late-publication/result-plan contracts in
  `backend_prepare_frame_stack_call_contract`; and the narrow x86
  `ConsumedPlans` scalar call-argument compatibility row in
  `backend_x86_handoff_boundary`.
- Result-lane identity is nearby same-feature coverage, not part of the
  selected implementation row unless the supervisor opens a separate packet.

## Proof

No build or CTest was run. This was a docs/lifecycle selection packet with no
implementation changes. Source inspection and AST-backed function queries were
used to select the reader and recommend the future narrow proof subset.
