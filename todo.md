Status: Active
Source Idea Path: ideas/open/287_post_286_prepared_boundary_interface_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Decide AArch64 Variadic HFA Lane Expansion Boundary

# Current Packet

## Just Finished

Completed Step 4 AArch64 variadic HFA lane expansion boundary decision. No
implementation files were changed.

Decision:

- Current owner:
  - `src/backend/bir/lir_to_bir/calling.cpp` owns the inline expansion today.
    `append_aarch64_variadic_hfa_carrier_arg_lanes` verifies an AArch64
    variadic call, classifies aggregate layout with
    `collect_va_arg_hfa_payload_lanes`, resolves the source aggregate alias and
    local aggregate leaf slots, then appends one BIR argument per lane with
    per-lane `CallArgAbiInfo::aarch64_hfa_lane_count` and
    `aarch64_hfa_lane_index`.
  - The same target policy is also visible in `aapcs64_va_arg_hfa_payload_shape`
    for aggregate `va_arg` lowering, which publishes
    `CallInst::va_arg_payload_abi`, `va_arg_hfa_lane_count`, and
    `va_arg_hfa_lane_size_bytes` for prepared variadic-entry planning.
  - Downstream prepared/prealloc code already consumes those fields as lowered
    ABI facts. It should not rediscover HFA shape from rendered type text or
    local aggregate slot structure.

- Boundary decision:
  - Keep the existing inline semantic BIR expansion temporarily. It is the
    behavior-preserving compatibility owner for the completed 286 route until
    a helper extraction can preserve every fail-closed check.
  - The desired owner is a target ABI helper, not general semantic call
    lowering. Semantic BIR should provide structured input facts and attach the
    returned lowered arguments/ABI records to `CallInst`; it should not embed
    AAPCS64 lane-count, lane-size, register-bank, leaf-slot, or HFA eligibility
    policy inline.
  - `CallArgAbiInfo` remains the lowered ABI-record contract for call
    arguments. A helper may produce those records, but it should not introduce a
    parallel ABI metadata shape that downstream prepared/prealloc must merge
    with `CallArgAbiInfo`.
  - `CallInst::va_arg_payload_abi`, `va_arg_hfa_lane_count`, and
    `va_arg_hfa_lane_size_bytes` remain the aggregate `va_arg` output contract
    for prepared variadic-entry planning unless a later helper deliberately
    replaces them with an equivalent structured payload plan.

Structured helper contract for a future extraction:

- Input:
  - `TargetProfile`, with AArch64 and floating-register availability checked by
    the helper.
  - A variadic-call marker and the original aggregate source operand.
  - Structured aggregate layout from `LirTypeRef`/`StructNameId` plus the
    backend structured layout table, not rendered suffix parsing.
  - Local aggregate alias and leaf-slot facts sufficient to map each payload
    lane to a BIR source value.
  - Local scalar slot types used to fail closed when a leaf slot does not match
    the classified HFA lane type.
- Output:
  - For outgoing calls, either `std::nullopt` for "not an AAPCS64 HFA carrier"
    or an ordered lane expansion containing `bir::Value` lane sources,
    lane `bir::TypeKind`s, and one `bir::CallArgAbiInfo` per lane with
    `aarch64_hfa_lane_count` and `aarch64_hfa_lane_index` populated.
  - For aggregate `va_arg`, either no HFA payload shape or a structured payload
    shape containing lane type, lane count, lane size, and payload ABI facts
    that map directly onto the current `CallInst` fields.
  - Failure should be explicit and fail closed for incomplete structured
    layout, missing aggregate alias, leaf-slot count mismatch, missing local
    slot type, or lane type mismatch. It must not silently fall back to a
    non-HFA byval pointer path after partially recognizing an HFA carrier.

Follow-up idea text if this route is too large for Step 6:

```text
Title: Extract AAPCS64 variadic HFA lane expansion into a target ABI helper

Intent:
Move the AArch64 variadic HFA carrier-array expansion policy out of semantic
call lowering into a target ABI helper that accepts structured aggregate layout,
aggregate alias, leaf-slot, and local slot type facts, then returns lowered BIR
lane values plus `CallArgAbiInfo` records. Preserve current aggregate `va_arg`
payload metadata behavior and all 286 fail-closed checks.

Owned files:
- `src/backend/bir/lir_to_bir/calling.cpp`
- a new or existing target ABI helper module under `src/backend/bir/lir_to_bir/`
  or an adjacent target ABI policy location
- focused backend tests around AAPCS64 variadic HFA outgoing calls and
  aggregate `va_arg` metadata

Proof:
- build proof
- `backend_lir_to_bir_notes`
- `backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`
- `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
- focused unit proof that mismatched leaf-slot count/type and missing aggregate
  alias still fail closed

Reject signals:
- extraction only moves the lambda body without defining structured input and
  output contracts
- helper consumes rendered call-argument/type suffix text as its primary owner
- helper emits lane values without matching `CallArgAbiInfo` records
- semantic BIR or prepared/prealloc recomputes HFA lane shape after the helper
  has already classified it
- a recognized-but-incomplete HFA carrier silently degrades to byval pointer
  lowering instead of failing closed
```

## Suggested Next

Proceed to Step 5. Decide the opaque pointer byte-offset provenance boundary:
whether prepared `memory_accesses` can keep the current
`allow_opaque_ptr_base && stored_type == I8` compatibility rule or need an
explicit base-object extent / byte-range carrier first.

## Watchouts

Do not use Step 4 as a license for broad AArch64 ABI rewrite. A safe helper
extraction must be behavior-preserving and must keep the current lane metadata
contract with prepared/prealloc intact. If Step 6 chooses only one cleanup, the
Step 3 structured `alignstack` carrier is likely narrower than extracting the
HFA helper because the HFA path crosses semantic BIR, local aggregate slots,
`CallArgAbiInfo`, and variadic-entry planning.

## Proof

Not run (decision-only). No `test_after.log` was created because this packet
made only documentation/state updates in `todo.md`.
