Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate The Affected Helper Boundary

# Current Packet

## Just Finished

Step 3 of `plan.md` verified the affected byval lane helper boundary after the
selected `byval_register_lane_size_bytes` authority path was removed. Repo-wide
search finds the retired helper only in planning text, AST lookup finds no
declaration or definition in `calls_byval_aggregates.cpp`, and the replacement
`prepared_byval_lane_extent_bytes` helper is an anonymous-namespace
emission-only helper in `calls_moves.cpp` called only by `lower_before_call_move`.

## Suggested Next

Supervisor should decide the next lifecycle action for the active runbook.

## Watchouts

- No additional narrow consolidation is required for the selected
  `byval_register_lane_size_bytes` boundary: it is fully retired from the code
  boundary rather than left as a cross-file helper.
- The remaining byval lane-size calculation is deliberately emission-local in
  `calls_moves.cpp`; broader byval helper reads of `CallInst::arg_abi` remain
  outside this selected boundary.

## Proof

No code changes were needed, so no build was required by the delegated proof
contract. Existing accepted Step 2 broad proof remains `test_before.log`:
162/162 `^backend_` tests passed.
