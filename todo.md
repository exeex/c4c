# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.18
Current Step Title: Receive the checked builtin-clz call/narrow/final-use

## Just Finished

- Step 7.17 accepted (`701476a5a`): received only producer-verified
  undefined-zero builtin-ctz Cttz results: i32 through one exact later i32 Add,
  and i64 through one exact i64-to-i32 Trunc then i32 Add. Matching regression
  guard was non-decreasing at 2/2; fresh build and broader `^backend_` proof
  passed 4/4.

## Suggested Next

- Execute only Step 7.18: PI's i32/i64 builtin-clz Ctlz call result, exact i32
  Add or i64-to-i32 Trunc-to-Add final-use chain. Do not widen into ctz, ffs,
  popcount, or prepared-argument receipt.

## Watchouts

- The clz receipt relies only on typed LIR IDs, fixed signatures, undefined-zero
  behavior, and immediate flag authority; callee/result/argument display fields
  remain non-authoritative. Missing-authority rows remain separately scoped.

## Proof

- Required fresh build: `cmake --build --preset default`. Focused proof:
  `ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'`;
  must pass 2/2. The supervisor selects the matching regression guard and
  broader checkpoint.
