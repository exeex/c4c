Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Retained Metadata Authority Leak

# Current Packet

## Just Finished

Step 1 of `plan.md` selected `byval_register_lane_size_bytes` in
`src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp` as the next
retained metadata authority leak to remove. The retained read is the helper's
lookup of the current `bir::CallInst`, then `call->arg_abi[*move.destination_abi_index]`,
to recheck pointer/byval/sret/register-or-stack/integer-class shape and return
`arg_abi.size_bytes` for AArch64 byval aggregate register-lane emission.

## Suggested Next

Execute Step 2 of `plan.md`: delete the local `byval_register_lane_size_bytes`
decision path and consume prepared facts instead. The prepared replacement
authority is `PreparedMoveResolution` classification through
`is_aarch64_byval_register_lane_move(move)` / reason
`call_arg_byval_aggregate_register_lanes`, plus the matched
`PreparedCallArgumentPlan` and `PreparedValueHome::size_bytes` already used by
the call-argument publication branches as the lane byte width.

## Watchouts

- Expected deletion path: remove the declaration from `calls.hpp`, remove the
  definition from `calls_byval_aggregates.cpp`, remove the
  `aggregate_lane_size` computation in `calls_moves.cpp`, and let the existing
  lane-size guards use `source_home->size_bytes` after the prepared
  byval-lane move classification has matched.
- Retained BIR access for instruction identity or diagnostics may remain, but
  the selected path should no longer inspect `CallInst::arg_abi` to decide
  byval lane shape or byte width.
- If `source_home->size_bytes` is not complete enough for any matched branch,
  the precise missing prepared fact is an explicit byval register-lane byte
  width on the prepared move or call-argument plan; do not recreate the
  `CallInst::arg_abi` decision locally.

## Proof

No build or ctest was required for this todo-only Step 1 selection packet.
Proposed Step 2 proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_instruction_dispatch$'`.
No `test_after.log` was produced for this packet.
