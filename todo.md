# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.19
Current Step Title: Receive the checked builtin-popcount call/narrow/final-use

## Just Finished

- Step 7.18 complete: received only producer-verified undefined-zero builtin-clz
  Ctlz results: i32 through one exact later i32 Add, and i64 through one exact
  i64-to-i32 Trunc then i32 Add. Raw-BIR builder and verifier admit only the
  typed i32 Ctlz undefined-zero direct final use; Ctpop remains excluded.

## Suggested Next

- Execute only Step 7.19: receive the producer-verified builtin-popcount i32/i64
  `Ctpop` result into one exact later i32 Add, or through one exact
  i64-to-i32 Trunc into that Add. Keep clz/ctz/ffs closed; do not receive
  parity, prepared arguments, or any other intrinsic/call/cast/binary row.

## Watchouts

- Resolve the Ctpop result, direct `LinkNameId`, fixed one-integer signature,
  result/narrowing IDs, and final-use edge through structured authority only.
  Ctpop must have no `zero_count_behavior`; callee/result/argument displays
  remain non-authoritative. Missing-authority rows remain separately scoped.

## Proof

- Landed Step 7.18 proof: fresh build `cmake --build --preset default`; focused
  2/2 `ctest --test-dir build -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'`; non-decreasing
  matching regression guard; and fresh broader `ctest --test-dir build -j
  --output-on-failure -R '^backend_'` 4/4. For Step 7.19, require a fresh build,
  matching focused transactional proof, and the supervisor-selected broader
  checkpoint. Use matching canonical before/after regression logs under
  supervisor control.
