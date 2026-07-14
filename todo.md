# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.3
Current Step Title: Receive the checked ffs Cttz-to-Add-one result

## Just Finished

- Step 7.2 complete: received only the authority-matrix i32 `LirAbsOp` row as
  a tagged Raw-BIR Abs opcode/payload/spec/view. The importer accepts exactly
  a current-function i32 result from the admitted selected-global i32 Load,
  keeps its ordered value identity, and permits the existing i32 Add use.

## Suggested Next

- Execute Step 7.3 only: receive the native i32 Cttz-result plus immediate-one
  Add row that precedes the still-unsupported ffs zero comparison and Select.

## Watchouts

- Abs is one tagged i32 receipt family, not a call or intrinsic route.
  `labs`/`llabs`, immediate inputs, non-i32/vector/aggregate forms, malformed
  typed linkage, presentation-derived recovery, and other builtin/call forms
  remain fail-closed.
- The repaired route is not a closure claim. Every remaining matrix row stays
  unsupported until its own bounded receiver packet or separately scoped
  producer-authority blocker is selected.

## Proof

- Step 6.5 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`.
- Step 7.1 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  no test log was changed by this packet.
- Step 7.2 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`.
