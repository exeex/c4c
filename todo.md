# Execution State

Status: Active
Source Idea Path: ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Re-Establish The Owned Publication/Layout Seam
Plan Review Counter: 2 / 5
# Current Packet

## Just Finished

Re-established plan step `1` for idea 76 with the delegated focused proof:
`c_testsuite_x86_backend_src_00204_c` still goes wrong first in `arg()`
before `Return values:` begins, and the regenerated
`build/c_testsuite_x86_backend/src/00204.c.s` confirms the current
`fa4(...)` setup overlaps published homes. At the call site, `arg()` first
publishes `s1` into `[rsp + 364]`, then publishes `hfa14` into
`[rsp + 352]`, `[rsp + 356]`, `[rsp + 360]`, and `[rsp + 364]`, so
`hfa14.d` overwrites the `s1` byte home immediately before `call fa4`.
That matches the still-bad mixed-argument print
`0 14.1 0.0 12 24.1 24.4 ...` and keeps ownership in idea 76's upstream
prepared-home publication/layout seam.

## Suggested Next

Take idea 76 step `2` by tracing one authoritative prepared-home publisher
for this mixed aggregate/HFA overlap:

- start from the `arg()` `fa4(...)` setup that aliases `s1` and `hfa14.d`
- isolate which prepared-home or stack-layout producer decided those
  overlapping homes
- keep the proving set narrow: rerun
  `c_testsuite_x86_backend_src_00204_c` and re-inspect
  `build/c_testsuite_x86_backend/src/00204.c.s` around `arg()` / `call fa4`
  while tracing the upstream publisher in `src/backend/prealloc/**`

## Watchouts

- Idea 75 does not own this first bad fact yet because the call-lane consumer
  is still being fed overlapping homes before `fa4` executes; fixing a later
  consumer/clobber route would accept false published inputs.
- Idea 77 does not own this first bad fact yet because the run is already
  wrong in `arg()` before `Return values:` begins; downstream return/HFA
  debugging is premature until this overlap moves.
- Keep the route classification upstream: this is a truthful-home publication
  or layout seam, not a `va_list` traversal issue and not a test-expectation
  problem.

## Proof

Fresh route-classification proof on 2026-04-22 used the delegated command:

- `cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^c_testsuite_x86_backend_src_00204_c$' | tee test_after.log`
- result: fails, with the first mismatch still inside `Arguments:` in `arg()`
  before `Return values:`
- proof log: `test_after.log`
- supporting inspection: `build/c_testsuite_x86_backend/src/00204.c.s` around
  the `arg()` `fa4(...)` setup and `call fa4`
