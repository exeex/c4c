# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.18
Current Step Title: Receive the checked builtin-clz call/narrow/final-use

## Just Finished

- Step 7.18 complete: received only producer-verified undefined-zero builtin-clz
  Ctlz results: i32 through one exact later i32 Add, and i64 through one exact
  i64-to-i32 Trunc then i32 Add. Raw-BIR builder and verifier admit only the
  typed i32 Ctlz undefined-zero direct final use; Ctpop remains excluded.

## Suggested Next

- Select the next source-matrix receiver-ready row; keep builtin-clz closed and
  do not widen it into ctz, ffs, popcount, or prepared-argument receipt.

## Watchouts

- The clz receipt relies only on typed LIR IDs, fixed signatures, undefined-zero
  behavior, and immediate flag authority; callee/result/argument display fields
  remain non-authoritative. Missing-authority rows remain separately scoped.

## Proof

- Fresh build: `cmake --build --preset default`. Focused 2/2 proof:
  `ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'`;
  passed. Log: `test_after.log`. The delegated focused proof is sufficient for
  this packet; broader checkpoint selection remains with the supervisor.
