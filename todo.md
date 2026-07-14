# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.17
Current Step Title: Receive the checked builtin-ctz call/narrow/final-use

## Just Finished

- Step 7.16 complete: proved the existing structured intrinsic receiver retains
  only i32/i64 native defined-zero Cttz results as the exact same-width Add-one
  lhs, with malformed result, callee/signature/behavior, and result-linkage
  authority rolling back transactionally (`cf7f2cd8d`).

## Suggested Next

- Execute repaired Step 7.17 only: receive PI's i32/i64 builtin-ctz native
  undefined-zero Cttz call, with its exact direct i32 Add use or exact
  i64-to-i32 Trunc-to-Add chain. Do not widen into ffs, clz, popcount, or
  prepared-argument receipt.

## Watchouts

- Closure was rejected because the source coverage matrix still has
  receiver-ready rows and separate producer-authority gaps. Step 7.17 is an
  in-scope repair; missing-authority rows remain fail-closed and require their
  own successor/blocker rather than presentation recovery.

## Proof

- Step 7.16 acceptance: `cmake --build --preset default`, then focused 2/2
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$`, plus matching
  non-decreasing regression guard and fresh `^backend_` 4/4 proof. Step 7.17
  must repeat its own fresh build, focused proof, transactional negatives, and
  supervisor-selected broader checkpoint.
