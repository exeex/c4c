# Current Packet

Status: Active
Source Idea Path: ideas/open/780_lir_cross_function_value_id_ownership_restoration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove nearby multi-function authority behavior

## Just Finished

- Step 2 — Repair the bounded shared ownership model: added the explicit
  module-wide native `LirValueId` allocator and routed `fresh_value`, direct
  label-address constants, and the two selected memcpy pointer-authority
  allocations through it. `FnCtx::lir_function` remains the current-function
  ownership/context binding; blocks and objects retain their per-function
  allocators, and the flag-gated fail-closed verifier contract is unchanged.
- Fresh build passed and `llvm_gcc_c_torture_src_pr52129_c` now passes: both
  functions lower and verify without a false foreign-owner collision.

## Suggested Next

- Step 3: replace the nearby obsolete per-function numeric-ID expectation with
  focused multi-function proof of module-unique native IDs plus malformed and
  foreign authority rejection.

## Watchouts

- `frontend_lir_call_type_ref` currently fails only because it asserts that
  separately lowered functions retain the same numeric load ID. That is the
  obsolete per-function namespace contract; Step 3 must migrate it without
  weakening verifier behavior or adding logical-only exceptions. Do not absorb
  PHI/generic producer migration.

## Proof

- `cmake --build --preset default` passed. The matching focused subset
  `ctest --test-dir build -j --output-on-failure -R
  '^(frontend_lir_call_type_ref|llvm_gcc_c_torture_src_pr52129_c)$'` has
  `llvm_gcc_c_torture_src_pr52129_c` passing and only the obsolete
  same-numeric-ID assertion in `frontend_lir_call_type_ref` failing. No
  canonical root log was written; Step 3 owns the proof migration.
