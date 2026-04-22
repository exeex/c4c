# Long-Double Aggregate Asm Emission For X86 Backend

Status: Closed
Created: 2026-04-22
Last-Updated: 2026-04-22
Closed: 2026-04-22
Disposition: Completed; the owned long-double aggregate asm-emission family no longer fails because the assembler rejects x86-emitted instructions and the surviving `00204.c` route graduated into a later post-assembly leaf.
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Make the x86 backend render long-double aggregate save/restore and related
helper paths into assembler-valid output once cases have already cleared
semantic lowering, prepared-module traversal, prepared call-bundle
consumption, and prepared local-slot handoff.

This leaf exists because some routes no longer fail on missing ownership or
bounded prepared traversal. They now reach final asm emission, but the x86
emitter still spells long-double aggregate instructions in a form the current
assembler rejects.

## Owned Failure Family

This idea owns backend failures where the route reaches final x86 assembly
rendering and then fails because generated long-double aggregate instructions
are rejected by the assembler, including the currently observed invalid
`fldt` / `fstpt` mnemonic seam.

## Current Known Failed Cases It Owns

- `c_testsuite_x86_backend_src_00204_c`

As additional backend cases are confirmed to fail for the same downstream
long-double aggregate asm-emission reason, they should route here instead of
being stretched back into idea 61's prepared-module ownership or idea 68's
prepared local-slot handoff ownership.

## Latest Durable Note

As of 2026-04-22, the active `00204.c` route still clears the bounded
multi-function prepared-module family that idea 61 owned, and the accepted
idea-69 packet cleared the remaining assembler-invalid long-double aggregate
mnemonic seam. Fresh proof now shows `00204.c` assembling successfully and
failing later at link time, while the paired
`backend_x86_handoff_boundary` proof also moved to a downstream stale
post-asm contract expectation for the multi-defined global-function-pointer /
indirect variadic-runtime boundary. Durable ownership therefore graduates out
of this asm-emission leaf and into
`ideas/open/70_post_asm_global_function_pointer_and_variadic_runtime_link_closure_for_x86_backend.md`.

## Scope Notes

Expected repair themes include:

- canonical x86 asm emission for long-double loads, stores, and aggregate
  materialization paths
- correct ownership between prepared value/home facts and the final x87 or
  long-double render lane
- explicit routing for cases that advance out of asm-emission failure and into
  later runtime-correctness ownership

## Boundaries

This idea does not own:

- semantic lowering failures before x86 prepared emission exists
- prepared call-bundle or multi-function prepared-module consumption; those
  remain in idea 61
- authoritative prepared local-slot handoff misses; those remain in idea 68
- generic scalar expression or terminator selection that fails before the
  long-double aggregate render lane is chosen; those remain in idea 60
- runtime correctness bugs after assembly succeeds; those belong in idea 63

## Completion Signal

This idea is complete when the owned cases no longer fail because the
assembler rejects x86-emitted long-double aggregate instructions and instead
either assemble successfully or graduate into a later, better-fitting leaf.
