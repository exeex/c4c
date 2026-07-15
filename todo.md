# Current Packet

Status: Active
Source Idea Path: ideas/open/783_lir_native_vaarg_operand_result_seam_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the focused frontend-LIR baseline

## Just Finished

- Lifecycle switch: preserved 782 Step 1 at its first `LirGepOp.ptr` raw-base
  blocker and activated the three-seam frontend-LIR decomposition route. No
  implementation slice or code change was accepted.

## Suggested Next

- Inventory the existing frontend-LIR tests, select the narrow structural
  baseline and three-chain probe locations, then record its fresh result.

## Watchouts

- `tests/backend/case/` is not the probe location for this frontend-LIR
  authority work; do not substitute backend or rendered-output assertions.
- Do not modify PHI carrier/verification or absorb Raw-BIR/importer, backend,
  target lowering, MIR, emission, generic migration, or text recovery.
- 782 resumes only after all AArch64 GP, AArch64 FP/alignment, and AMD64
  reg/stack contracts are accepted; 751 remains parked until 782 completes.

## Proof

- Establish a fresh focused frontend-LIR baseline in Plan Step 1. The outgoing
  782 baseline passed:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
