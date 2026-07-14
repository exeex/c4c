# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 6.5
Current Step Title: Receive the checked i32 SLT compare result

## Just Finished

- Step 6.4 complete in `39518d27a`: imported only explicit scalar
  `LirCastOp{SExt, i32, i64}` with current-function source/result `LirValueId`s
  as a typed Raw-BIR cast result, and preserved its exact downstream i64 `Add`
  use. Builder, verifier, and importer reject malformed ownership, linkage,
  endpoint, kind, duplicate, and unresolved-use authority transactionally.

## Suggested Next

- Execute Step 6.5 only: receive the producer-verified ordinary integer
  `LirCmpOp{is_float: false, predicate: Slt, type: i32}` with a
  current-function selected-global Load lhs and representable immediate-seven
  rhs. Do not receive the coupled normalization `ZExt` or any other compare.

## Watchouts

- Step 6.4's cast exclusions remain fail-closed. For Step 6.5, predicate,
  integer mode/type, result identity, and operands must come from native LIR
  authority; the following monostate-result normalization `ZExt` is not a
  receiver input. Other predicates/types, floating, pointer, vector, complex,
  logical-helper, builtin, vaarg, statement, CFG, and text-derived compares
  remain fail-closed.

## Proof

- Step 6.4 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$' >
  test_after.log` (2/2); proof log: `test_after.log`.
- Step 6.5 proof: fresh build plus the supervisor-selected focused receiver
  proof, retaining `frontend_lir_call_type_ref` as the producer neighbor.
