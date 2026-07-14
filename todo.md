# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.1
Current Step Title: Prove mixed accepted-row dispatcher transactionality

## Just Finished

- Step 7.1 complete: added one mixed source-order fixture for the existing
  selected-global i32 Load, normalized i32 Add, explicit i32-to-i64 SExt, and
  selected i32 SLT dispatcher branches. It proves typed current-function
  result/use authority and that changing only the final compare predicate
  rejects the entire module without Raw-BIR publication.

## Suggested Next

- Supervisor: select the next packet; do not expand Step 7.1 beyond its
  existing-dispatcher coverage.

## Watchouts

- Step 7.1 added no instruction variant or broader receipt. The compare remains
  limited to a current-function selected-global i32 Load lhs, native immediate
  seven rhs, `Slt`, and i32 integer mode; the following monostate-result `ZExt`
  and every other predicate/type/domain remain fail-closed.

## Proof

- Step 6.5 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`.
- Step 7.1 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  no test log was changed by this packet.
