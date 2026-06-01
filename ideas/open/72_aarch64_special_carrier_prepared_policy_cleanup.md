# AArch64 Special Carrier Prepared Policy Cleanup

## Goal

Make AArch64 i128 and f128 lowering consume prepared carrier, runtime-helper,
ABI, lane, resource, and clobber policy facts before emitting concrete
AArch64 pair, lane, vector, transport, and helper-call records.

## Why This Exists

The large-owner residue audit classified i128 and f128 carrier/helper policy
as `consume-shared`. Prepared authority already describes carrier selection,
runtime-helper policy, lane bindings, ABI transitions, call preservation,
resource ownership, scalar result ownership, memory-backed carrier facts, and
clobber policy. AArch64 should not re-decide that policy while lowering the
target-specific instruction sequences.

## Owned Files

- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- `src/backend/mir/aarch64/codegen/f128.cpp`
- Shared prepared authority call sites only where needed to consume existing
  facts:
  - `src/backend/prealloc/i128_runtime_helpers.*`
  - `src/backend/prealloc/f128_runtime_helpers.*`
  - `src/backend/prealloc/special_carriers.*`
  - relevant value-home, addressing, and regalloc prepared lookup surfaces

## In Scope

- Consume `PreparedI128Carrier*`, `PreparedI128RuntimeHelper*`,
  prepared lane bindings, ABI policy, call-preservation policy, clobber policy,
  and value-home/regalloc facts.
- Consume `PreparedF128Carrier*`, `PreparedF128RuntimeHelper*`,
  ABI transition/result ownership, resource/clobber policy, scalar result
  ownership, memory-backed carrier facts, addressing, and value homes.
- Keep AArch64 i128 pair/lane instruction selection, shift/count admissibility,
  compare sequence emission, Q/vector register conversion, f128 memory address
  spelling, helper-boundary records, and final transport/helper emission local.

## Out Of Scope

- Variadic entry-plan cleanup; that is a separate variadic follow-up.
- General aggregate transport planning; that belongs to the aggregate-transport
  evidence/proposal follow-up.
- Moving AArch64 pair/lane or vector instruction selection into shared
  prepared policy code.
- Reworking helper ABIs beyond consuming the already prepared policy.

## Proof Expectations

- Focused AArch64 i128 and f128 tests covering carrier selection,
  runtime-helper boundaries, lane bindings, memory-backed carriers, ABI
  transition/result ownership, and clobber/resource policy.
- Evidence that prepared special-carrier facts are consumed before target
  emission.
- Regression guard logs for acceptance-sized slices.

## Reviewer Reject Signals

- A patch still locally decides i128/f128 carrier policy, helper policy,
  clobbers, lane bindings, or ABI result ownership after claiming migration.
- A named i128 or f128 testcase is hard-coded instead of consuming prepared
  policy for the family.
- AArch64 pair/lane, vector, memory-address spelling, or helper-boundary record
  emission is moved into shared prealloc code.
- Tests or expectations are weakened to hide unchanged policy duplication.
- The route drags unrelated ALU, memory, call, or variadic cleanup into this
  special-carrier policy slice.
