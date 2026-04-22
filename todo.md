# Execution State

Status: Active
Source Idea Path: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Re-Establish The Downstream Consumer Ownership
Plan Review Counter: 0 / 5
# Current Packet

## Just Finished

Lifecycle repair switched the active runbook from idea 76 to idea 75 after the
latest step-2 trace proved `00204.c` no longer fails in an upstream
publication/layout seam. The durable facts preserved into the source ideas are:
`stack_layout::assign_frame_slots(...)` publishes `%t37.0` at `364` and
`%t38.0/.4/.8/.12` at `80/84/88/92`; regalloc publishes distinct late homes at
`6264`, `6272`, and `6376/6380/6384/6388`; the prepared `fa4(...)` move bundle
uses register ABI destinations only; and the emitted x86 still collapses those
truthful facts into an overlapping write at `[rsp + 364]` before `call fa4`.
That makes the current packet a downstream prepared-to-x86 consumer problem,
not another upstream publisher/layout search.

## Suggested Next

Re-establish idea 75 ownership in packet form:

- confirm the first mismatch still occurs in `arg()` before `Return values:`
- restate the truthful prepared offsets versus the emitted `[rsp + 352..364]`
  overlap at `fa4(...)`
- set the next seam on the x86 lowering consumers in
  `prepared_local_slot_render.cpp` and `x86_codegen.hpp`

## Watchouts

- Do not reopen `src/backend/prealloc/**` unless fresh proof contradicts the
  current truthful publisher facts.
- Do not treat idea 77 as active while the first bad fact is still a pre-call
  overlap before `fa4(...)` executes.
- Reject callsite-shaped fixes that only special-case `00204.c` instead of
  repairing the shared x86 lowering contract.

## Proof

Fresh proof carried forward into this lifecycle repair on 2026-04-22 used:

- `cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^c_testsuite_x86_backend_src_00204_c$' | tee test_after.log`
- result: still fails, with the first mismatch still inside `Arguments:` in
  `arg()` before `Return values:`
- proof log: `test_after.log`
- supporting inspection: `build/c_testsuite_x86_backend/src/00204.c.s` around
  the `arg()` `fa4(...)` setup plus the focused prepared-BIR dump showing
  `prepared.func @arg frame_size=376`, stack-layout slots `#718-#721` and
  `#990`, regalloc homes `#2440-#2441` and `#2461-#2464`, and the
  `move_bundle phase=before_call block_index=0 instruction_index=623`
  register-only ABI destinations for `fa4(...)`
