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

- `c_testsuite_x86_backend_src_00204_c`

## Latest Durable Note

As of 2026-04-22, fresh focused proof

`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'`

shows `backend_x86_handoff_boundary` passing while `00204.c` still fails at
runtime with `[RUNTIME_MISMATCH]` after idea 74's owner-publication repair.
Dumped BIR and prepared BIR now show truthful semantic owners:
`fa_s1` through `fa_s16` call `printf(..., ptr %lv.param.a.0)` and `fa_s17`
calls `printf(..., ptr %p.a)`. Generated
`build/c_testsuite_x86_backend/src/00204.c.s`, however, still emits
`lea rdi, [rip + .L.str16]` followed by `mov rsi, rdi` in `fa_s17`, so arg1 is
copied from the clobbered format-string register instead of preserving `%p.a`.
The same prepared/x86 call-lane clobber also appears in `pll`, so durable
ownership graduates out of idea 74 and into this downstream post-link
call-lane runtime leaf.

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
