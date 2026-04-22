# Execution State

Status: Active
Source Idea Path: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Re-Establish The Downstream Consumer Ownership
Plan Review Counter: 1 / 5
# Current Packet

## Just Finished

Plan step 1 is now re-established with fresh proof on `00204.c`: the test still
fails first inside `Arguments:` in `arg()` before `Return values:`. The active
consumer seam remains the `fa4(...)` setup in
`build/c_testsuite_x86_backend/src/00204.c.s`, where the truthful prepared/home
facts already carried forward from idea 76 stay distinct upstream
(`%t37.0 -> 364`, `%t38.0/.4/.8/.12 -> 80/84/88/92`, late homes at `6264`,
`6272`, and `6376/6380/6384/6388`), but the emitted x86 still stages the call
through `[rsp + 352..364]` and overwrites `[rsp + 364]` twice before `call fa4`
at line `3493`: first `s1` byte storage at line `3452`, then `hfa14.d` at line
`3469`. That keeps ownership in idea 75's downstream prepared-to-x86 consumer
route, not in idea 76's already-truthful publishers or idea 77's later
return/HFA route.

## Suggested Next

Inspect the downstream x86 consumer seam that turns those truthful prepared
homes into the emitted `[rsp + 352..364]` staging area before `fa4(...)`,
starting with `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` and
`src/backend/mir/x86/codegen/x86_codegen.hpp`, while keeping the proving set
narrow to `00204.c` plus the prepared/assembly evidence around that call site.

## Watchouts

- Do not reopen `src/backend/prealloc/**`: fresh proof still shows the first bad
  fact only after the truthful upstream homes are consumed into emitted x86
  call setup.
- Do not hand route ownership to idea 77 while the earliest mismatch remains in
  `arg()` before `fa4(...)` and before `Return values:`.
- Reject any fix that only reshapes this single call site instead of repairing
  the shared x86 consumer contract that materializes byval/local call lanes.

## Proof

Fresh proof on 2026-04-22 used:

- `cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^c_testsuite_x86_backend_src_00204_c$' | tee test_after.log`
- result: still fails, with the first mismatch still inside `Arguments:` in
  `arg()` before `Return values:`
- proof log: `test_after.log`
- supporting inspection:
  `build/c_testsuite_x86_backend/src/00204.c.s:3451-3493`, where the emitted
  call setup stores `s1` to `[rsp + 364]`, then rewrites that same location as
  the tail of the staged HFA copy while using `[rsp + 352]` as the `fa4(...)`
  aggregate base
