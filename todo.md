Status: Active
Source Idea Path: ideas/open/288_extract_aapcs64_variadic_hfa_lane_expansion_helper.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Current HFA Expansion Inputs and Fail-Closed Edges

# Current Packet

## Just Finished

Step 1 inventory completed for the outgoing AAPCS64 variadic HFA
carrier-array expansion currently embedded in
`src/backend/bir/lir_to_bir/calling.cpp`.

## Structured Inputs Observed

- Call context: `context_.target_profile`, `is_variadic_call`, call argument
  index, and the source `LirOperand`.
- Aggregate shape: `AggregateTypeLayout` from `lower_byval_call_arg_layout`
  or `aggregate_alias_layout`; structured paths use `LirTypeRef` /
  `StructNameId` when present and legacy rendered byval text only through the
  existing compatibility bridges.
- HFA classification facts: `collect_va_arg_hfa_payload_lanes(...)` returns
  uniform FP lane type plus lane count for scalar, array, and struct layouts.
- Aggregate storage facts: `aggregate_value_aliases` maps the SSA carrier
  operand to a local aggregate slot; `local_aggregate_slots` provides the
  aggregate slot tree; `collect_sorted_leaf_slots(...)` orders leaf slots.
- Lane type facts: `local_slot_types` must contain each leaf slot and match
  the classified HFA lane type.
- ABI metadata sources: `compute_call_arg_abi(context_.target_profile,
  lane_type)` creates each lane `CallArgAbiInfo`; the first lane inherits
  `call.structured_args[index].aarch64_stack_align_bytes` when available,
  otherwise the aggregate layout alignment.

## Helper Contract Candidates

- Request should carry target profile, variadic-call marker, argument index,
  source operand identity, aggregate layout, aggregate alias map view, local
  aggregate slot map view, local scalar slot type map view, and optional
  structured call-argument metadata for alignment.
- Result should distinguish no expansion from an ordered expansion. The
  ordered expansion should return lane values, lane BIR types, and matching
  `bir::CallArgAbiInfo` records together so generic call lowering only appends
  the already-classified lanes.
- A rejected/incomplete HFA candidate should remain fail-closed at the call
  site. Step 2 should decide whether the helper reports an explicit rejection
  reason for tests/debugging or uses an empty/no-expansion result while the
  caller preserves the current failure behavior.

## Fail-Closed Edges To Preserve

- Not AArch64, not a variadic call, or non-SSA carrier operand: no HFA carrier
  expansion.
- Layout is not a valid HFA payload: invalid nested layout, empty arrays or
  structs, non-F32/F64/F128 scalar leaf, mixed lane types, zero lanes, or more
  than four lanes.
- Missing aggregate alias for the carrier SSA operand.
- Alias target missing from `local_aggregate_slots`.
- Sorted aggregate leaf-slot count differs from the classified HFA lane count.
- Missing local scalar slot type for any leaf slot.
- Local scalar slot type differs from the classified HFA lane type.
- `compute_call_arg_abi(...)` fails for the lane type.

## Existing Focused Proof

- `backend_lir_to_bir_notes` includes
  `expect_aarch64_variadic_carrier_array_call_uses_hfa_fp_lanes`, which checks
  the positive carrier-array path expands to F32 lanes, attaches
  `aarch64_hfa_lane_count` / `aarch64_hfa_lane_index`, and does not leave a
  byval carrier.
- `backend_lir_to_bir_notes` also covers direct/variadic fixed HFA calls,
  aggregate HFA local copy, HFA return/call-result, and HFA aggregate
  `va_arg` helper handoff.
- `backend_prepare_frame_stack_call_contract_test.cpp` includes
  `check_aarch64_f128_hfa_va_arg_metadata_contract`, proving aggregate
  `va_arg` payload metadata preserves F128 HFA lane facts.
- The completed 286 stdarg subset remains the main behavior-preserving proof:
  `backend_lir_to_bir_notes`,
  `backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`, and
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.

## Coverage Gaps For Later Steps

- No focused test was found for missing aggregate alias on a recognized HFA
  carrier operand.
- No focused test was found for leaf-slot count mismatch after HFA
  classification.
- No focused test was found for local leaf-slot scalar type mismatch or missing
  local slot type.
- Step 4 should add these only if the fixture can create the incomplete facts
  semantically; do not add testcase-shaped shortcuts or weaken existing
  supported-path expectations.

## Suggested Next

Execute `Step 2: Define the Target ABI Helper Contract`.

## Watchouts

- Keep idea 289 out of scope for this plan.
- Do not broaden AArch64 ABI classification or prepared/prealloc call-plan
  behavior.
- Do not claim progress by moving the current body into a helper without a
  structured input/output contract.
- Do not weaken supported-path test expectations or convert recognized
  incomplete HFA carriers into silent byval pointer lowering.

## Proof

Not run; Step 1 was inventory-only and changed only `todo.md`.
