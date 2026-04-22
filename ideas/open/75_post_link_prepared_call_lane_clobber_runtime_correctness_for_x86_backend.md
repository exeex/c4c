# Post-Link Prepared Call-Lane Clobber Runtime Correctness For X86 Backend

Status: Open
Created: 2026-04-22
Last-Updated: 2026-04-22
Parent Idea: [74_post_link_byval_param_pointer_argument_runtime_correctness_for_x86_backend.md](/workspaces/c4c/ideas/closed/74_post_link_byval_param_pointer_argument_runtime_correctness_for_x86_backend.md)

## Intent

Repair the next post-link x86 backend runtime leaf revealed by `00204.c`: the
byval-param pointer-owner publication seam is cleared, but emitted fixed-arity
call lanes still clobber overlapping argument homes after semantic ownership is
already truthful.

## Owned Failure Family

This idea owns x86 backend failures where:

- the prepared-module route already matches
- final assembly is already valid and the program links
- earlier post-link byval-param pointer-owner publication seams already cleared
- the next blocker is runtime correctness for prepared/x86 fixed-arity call
  lanes whose emitted move order copies an argument from a register that an
  earlier argument lane already rewrote

## Current Known Failed Cases It Owns

- none currently confirmed; `c_testsuite_x86_backend_src_00204_c` rehomed on
  2026-04-22 after step `2.2` proved the first remaining aggregate-string /
  mixed-aggregate corruption was upstream prepared byval-home
  publication/layout, not the helper-lane consumer contract this idea owns

## Latest Durable Note

As of 2026-04-22, step `2.2` of the active runbook classified the first
remaining same-module aggregate-string / mixed-aggregate corruption out of this
leaf. Focused proof still shows `backend_x86_handoff_boundary` passing while
`c_testsuite_x86_backend_src_00204_c` fails with `[RUNTIME_MISMATCH]`, but the
bounded helper lane in `prepared_local_slot_render.cpp` only consumes already
published byval homes from `PreparedModuleLocalSlotLayout`,
`PreparedValueHome`, and `PreparedStackLayout`; it does not author new payload
spacing. Generated `build/c_testsuite_x86_backend/src/00204.c.s` confirms the
first bad fact now exists before the helper call: the `fa1` / `fa2`
aggregate-string lane publishes overlapping stack homes at `rsp+6176`,
`rsp+6184`, `rsp+6192`, and `rsp+6200`, each acting as both the tail of one
aggregate copy and the base of the next. Any generic helper-consumer change at
this point either breaks the current `backend_x86_handoff_boundary` contract
or corrupts `00204.c` earlier. Durable ownership for `00204.c` therefore
graduates upstream into a dedicated prepared byval-home publication/layout
initiative, and idea 75 stays open only for future cases whose first bad fact
is still a downstream prepared/x86 call-lane consumer clobber seam after
homes are already published truthfully.

## Scope Notes

Expected repair themes include:

- truthful runtime semantics for overlapping fixed-arity call argument homes
  once codegen already succeeds
- correct BeforeCall move ordering or temporary preservation when prepared call
  lanes alias caller-saved ABI registers
- focused runtime proof that distinguishes call-lane clobber from earlier
  owner-publication seams and later variadic traversal work

## Boundaries

This idea does not own:

- byval-param pointer-owner publication; that was idea 74
- prepared-module or prepared call-bundle rejection before assembly; those
  remain in idea 61
- genuine post-link variadic traversal defects in `llvm.va_start`, `va_list`,
  or later `myprintf` walking; those remain in idea 71
- generic non-call runtime correctness work unrelated to the graduated
  fixed-arity call-lane route

## Completion Signal

This idea is complete when the owned post-link prepared/x86 call-lane clobber
cases no longer fail at their current overlapping argument-home seam and
instead either execute correctly or graduate into a later, better-fitting
runtime leaf.
