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

As of 2026-04-22, fresh lifecycle repair brings `00204.c` back into this leaf.
The latest focused proof still fails first inside `arg()` before `fa4(...)`
executes, but step-2 tracing on the upstream idea proved the authoritative
publishers are already truthful: `stack_layout::assign_frame_slots(...)`
publishes separate local offsets (`364` vs `80/84/88/92`), regalloc publishes
separate late homes (`6264`, `6272`, and `6376/6380/6384/6388`), and the
prepared move bundle uses only register ABI destinations for the direct
mixed aggregate/HFA call. The final emitted x86 still overwrites `[rsp + 364]`
before `call fa4`, so the remaining defect is downstream of prepared-home
publication and belongs to the consumer that lowers truthful `PreparedValueHome`
and `PreparedAddressingFunction` facts into final x86 addresses and call-lane
copies.

Keep this idea focused on that downstream consumer seam. The active packet
should inspect `prepared_local_slot_render.cpp`, `x86_codegen.hpp`, and any
adjacent x86 lowering helper that turns the truthful prepared offsets above
into the overlapping `[rsp + 352..364]` writes seen in
`build/c_testsuite_x86_backend/src/00204.c.s`. Idea 77 remains a later leaf
only after the first bad fact moves past this pre-call consumer clobber.

Also as of 2026-04-22, this leaf is no longer the right active runbook for the
next move. The repeated collisions around `prepared*.cpp` exposed a subsystem
ownership problem across the whole `src/backend/mir/x86/codegen/` directory:
the prepared route keeps growing side-channel lowering logic instead of using
the canonical x86 codegen seams already present in neighboring `.cpp` files.
That structural problem now has its own four-stage Phoenix rebuild sequence in
ideas 78 through 81. Keep idea 75 open as the durable symptom bucket for the
call-lane/runtime failure family, but park active execution here until the
rebuild sequence extracts and reviews the x86 codegen subsystem honestly.

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
