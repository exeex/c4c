# Post-Asm Global-Function-Pointer And Variadic-Runtime Link Closure For X86 Backend

Status: Closed
Created: 2026-04-22
Last-Updated: 2026-04-22
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Repair the first post-assembly x86 backend failures that remain after the
prepared-module restriction and long-double aggregate asm-emission leaves are
cleared for `00204.c`.

This leaf exists because `00204.c` now assembles successfully, but the route
still fails later on the same downstream family: the multi-defined
global-function-pointer and indirect variadic-runtime boundary no longer has
the right post-asm contract, and the emitted program still fails at link time
on unresolved same-module/global and variadic-runtime references.

## Owned Failure Family

This idea owns x86 backend failures where:

- the bounded multi-function prepared-module route already matches
- final assembly is already valid
- the next blocker is post-assembly closure for the
  global-function-pointer / indirect variadic-runtime family, including stale
  boundary-contract expectations and link-time unresolved references such as
  same-module `s1`..`s16` symbols or `llvm.va_start.p0`

## Current Known Failed Cases It Owns

- `backend_x86_handoff_boundary`

## Latest Durable Note

As of 2026-04-22, idea 69's owned long-double aggregate asm-emission seam is
cleared: `00204.c` now assembles and the generated asm no longer contains the
old invalid `fldt` / `fstpt` forms. Fresh proof first failed later on two
downstream surfaces from the same family:

- `backend_x86_handoff_boundary` reports a stale wrong-contract expectation
  for the multi-defined global-function-pointer and indirect variadic-runtime
  boundary
- `c_testsuite_x86_backend_src_00204_c` reaches link time and fails on
  unresolved references including `s1`..`s16` and `llvm.va_start.p0`

Durable ownership therefore graduates out of idea 69 and into this new
post-assembly closure leaf.

Later on 2026-04-22, the first post-assembly closure repair removed those
unresolved same-module and variadic-runtime references for `00204.c`. That
case now links and fails later at runtime with a variadic-path segfault, which
is no longer owned here and must be tracked as a separate runtime initiative.
The remaining currently owned seam for idea 70 is the truthful
`backend_x86_handoff_boundary` contract wording for the post-assembly closure
family.

## Scope Notes

Expected repair themes include:

- canonical post-assembly ownership for same-module symbol definitions once the
  prepared-module lane already matched
- canonical lowering or closure for indirect variadic-runtime references after
  assembly succeeds
- truthful boundary coverage for the current downstream contract once the
  family is no longer a rejection-only prepared-module case

## Boundaries

This idea does not own:

- prepared-module or prepared call-bundle rejection before assembly; those
  remain in idea 61
- assembler-invalid long-double aggregate emission; that was idea 69
- generic semantic-lowering or prepared local-slot handoff failures upstream of
  codegen
- pure runtime wrong-result or crash debugging after the program already links
  and executes; record that as a later runtime leaf instead of stretching this
  one

## Completion Signal

This idea is complete when the owned cases no longer fail in this post-assembly
global-function-pointer / indirect variadic-runtime closure family and instead
either link successfully or graduate into a later, better-fitting runtime
correctness leaf.

## Closure Note

Closed on 2026-04-22 after the owned post-assembly family was exhausted:

- `c_testsuite_x86_backend_src_00204_c` no longer fails on unresolved
  same-module or direct variadic-runtime references and now routes into the
  separate post-link runtime leaf tracked by idea 71
- `backend_x86_handoff_boundary` now passes with assertions aligned to the
  truthful current thrown rejection surface and separate route-debug summary
