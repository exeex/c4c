# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.4
Current Step Title: Receive the checked fixed-void external double call result

## Just Finished

- Step 7.3 complete: received only the native i32 `Cttz` result followed by
  its immediate-one i32 Add (`2f7055845`). The importer, Raw-BIR builder, and
  verifier keep the ordered source identity and require the Add lhs to be that
  admitted i32 Cttz result. The ffs zero comparison and Select remain
  unsupported because their comparison lhs has no typed LIR value carrier.

## Suggested Next

- Execute Step 7.4 only: receive the authority-matrix Step-7.32 resolved,
  zero-argument external double `LirCallOp` result and its exact later double
  `FAdd` use. Do not receive the FAdd itself, argument-bearing/variadic/
  indirect calls, or any ffs comparison or Select.

## Watchouts

- The next call is authorized by
  `docs/lir_remaining_ordinary_value_identity/authority_matrix.md` Step 7.32:
  valid current-function result ID, module `LinkNameId` shared with exactly one
  fixed-void external Function declaration, exact native double return type,
  empty fixed-void signature, and exact downstream `FAdd` lhs ID. Missing,
  duplicate, unresolved, cross-owner, signature/type, or result/use-linkage
  authority fails closed. The ffs zero comparison and Select remain unsupported
  because their lhs lacks a typed receiving carrier.

## Proof

- Step 7.3 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`. Step 7.4 requires the same fresh focused 2/2
  proof, plus the supervisor-owned regression gate before any later closure.
