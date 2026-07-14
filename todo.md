# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 6.4
Current Step Title: Receive the checked explicit i32-to-i64 SExt result

## Just Finished

- Step 6.4 complete: imported only explicit scalar
  `LirCastOp{SExt, i32, i64}` with current-function source/result `LirValueId`s
  as a typed Raw-BIR cast result, and preserved its exact downstream i64 `Add`
  use. Builder, verifier, and importer reject malformed ownership, linkage,
  endpoint, kind, duplicate, and unresolved-use authority transactionally.

## Suggested Next

- Supervisor selects the next bounded active-plan packet; do not broaden the
  accepted Cast receipt beyond the exact i32-to-i64 `SExt` row.

## Watchouts

- Other integer widths/kinds, no-op, truncation beyond the previously accepted
  intrinsic row, pointer, bitcast, floating, vector, aggregate, implicit, and
  presentation-derived casts remain fail-closed. The direct-branch display
  shadow contract remains unchanged.

## Proof

- Step 6.4 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$' >
  test_after.log` (2/2); proof log: `test_after.log`.
