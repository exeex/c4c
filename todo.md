# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.3.2
Current Step Title: Retry the resolved fixed-void native-floating Call importer row

## Just Finished

- Plan Step 5.3.1 admitted source-backed direct `CallSpec` results only for
  native `F32`/`F64` callee returns alongside the existing integer path. The
  builder preserves void no-result, exact signature type, source-owner/index,
  instruction-result linkage, and rollback behavior; focused builder coverage
  proves F32/F64 publication plus void-result and duplicate-source rejection.

## Suggested Next

- Execute Plan Step 5.3.2 only: retry the existing `aeecf048c` resolved
  fixed-void native-floating `LirCallOp` importer packet now that the Raw-BIR
  CallSpec prerequisite is available. Its downstream double `LirBinOp FAdd`
  remains a source-chain guard only, not a receipt target.

## Watchouts

- This prerequisite changes only the native Raw-BIR `CallSpec` result gate; it
  does not itself admit an importer row. Indirect, variadic, argument-bearing,
  ABI-expanded, unresolved, nonmatching coercion, aggregate/object, and every
  other floating-call form remain fail-closed. Do not import scalar FAdd.

## Proof

- Plan Step 5.3.1 passed:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_lir_to_bir_interface$' > test_after.log`.
  `test_after.log` is the matching after-proof.
- Plan Step 5.3.2 proof:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'
  > test_after.log`.
