# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.1
Current Step Title: Prove mixed accepted-row dispatcher transactionality

## Just Finished

- Step 6.5 complete: received only native
  `LirCmpOp{is_float: false, predicate: Slt, type: i32}` using the exact
  selected-global i32 Load result and immediate seven as a typed Raw-BIR
  i1 Compare result. Builder, verifier, and importer preserve operands,
  current-function source linkage, and transactional failure; the following
  monostate `ZExt` remains unsupported.

## Suggested Next

- Execute Step 7.1 only: add one mixed accepted-row module fixture that sends
  already admitted selected-global scalar Load, i32 Add, i32-to-i64 SExt, and
  i32 SLT forms through their existing dispatcher branches in source order.
  Mutate only the final admitted compare authority in a neighboring fixture and
  require whole-module rejection with no published Raw-BIR module. Do not add
  an instruction variant, broaden any row, or receive the following ZExt.

## Watchouts

- This is coverage of existing importer branches, not a new semantic receipt.
  The compare remains limited to a current-function selected-global i32 Load
  lhs, native immediate seven rhs, `Slt`, and i32 integer mode; the following
  monostate-result `ZExt` and every other predicate/type/domain remain
  fail-closed without presentation-text repair.

## Proof

- Step 6.5 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`.
- Step 7.1 proof: fresh build plus that same focused backend/producer command
  (2/2 expected); the backend fixture must prove ordered mixed receipt and
  rollback when its final already-admitted compare authority is malformed.
