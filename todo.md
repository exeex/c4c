# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.17
Current Step Title: Receive the checked builtin-ctz call/narrow/final-use

## Just Finished

- Step 7.17 complete: received only producer-verified undefined-zero builtin-ctz
  Cttz results: i32 through one exact later i32 Add, and i64 through one exact
  i64-to-i32 Trunc then i32 Add. The interface proof covers transactional
  malformed intrinsic/result/flag/final-use and narrowing failures.

## Suggested Next

- Select the next source-matrix receiver-ready row; keep builtin-ctz closed and
  do not widen it into ffs, clz, popcount, or prepared-argument receipt.

## Watchouts

- The ctz receipt relies only on typed LIR IDs, fixed signatures, zero behavior,
  and immediate flag authority; callee/result/argument display fields remain
  non-authoritative. Missing-authority rows remain separately scoped.

## Proof

- Fresh build: `cmake --build --preset default`. Focused 2/2 proof:
  `ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'`;
  passed. Log: `test_after.log`. The delegated focused proof is sufficient for
  this packet; broader checkpoint selection remains with the supervisor.
