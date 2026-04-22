# Post-Link Return-Value And HFA Runtime Correctness For X86 Backend

Status: Open
Created: 2026-04-22
Last-Updated: 2026-04-22
Parent Idea: [76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md](/workspaces/c4c/ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md)

## Intent

Repair the next post-link x86 backend runtime leaf revealed by `00204.c`: the
upstream prepared byval-home publication/layout seam is cleared, but the
emitted program still misbehaves later in aggregate return-value / HFA runtime
handling.

## Owned Failure Family

This idea owns x86 backend failures where:

- the prepared-module route already matches and helper byval payload homes are
  already published truthfully
- final assembly is already valid and the program links
- the first remaining bad fact is a post-link runtime mismatch in the later
  aggregate return-value or HFA portion of execution, not in earlier byval
  argument publication or helper payload spacing
- fixing the case requires truthful runtime return / HFA semantics rather than
  reopening upstream publication/layout or generic variadic traversal ownership

## Current Known Failed Cases It Owns

- none currently confirmed; the same-day `00204.c` graduation was reverted
  after fresh focused proof showed the first bad fact still occurs earlier in
  direct mixed aggregate/HFA argument publication/layout

## Latest Durable Note

As of 2026-04-22, the source-idea split from idea 76 remains a plausible
downstream leaf, but the same-day activation was premature. Fresh focused
proof,

`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^c_testsuite_x86_backend_src_00204_c$'`

followed by inspection of
`build/c_testsuite_x86_backend/src/00204.c.s`, shows the current first bad
fact still occurs in `arg()` at direct mixed aggregate/HFA call `fa4(...)`:
the byte home for `s1` at `[rsp + 364]` is overwritten by `hfa14.d` before the
call executes. That means `00204.c` has not yet advanced into a truthful
downstream return-value / HFA runtime leaf.

Keep idea 77 open only as a future downstream bucket for cases whose first bad
fact really does move past argument publication/layout into post-link return
or HFA runtime handling. `00204.c` should remain outside this idea until that
boundary is actually reached.

## Scope Notes

Expected repair themes include:

- truthful post-link runtime return semantics for aggregate, floating, and HFA
  paths once helper argument publication is already correct
- correct x86 return-lane carrier handling for floating / aggregate fragments,
  including sret-adjacent or x87/SSE handoff details when those are the first
  bad fact
- route proof that distinguishes return-value / HFA runtime ownership from
  earlier helper payload publication defects and later variadic traversal work

## Boundaries

This idea does not own:

- upstream byval-home publication/layout overlap before the helper call; that
  remains in idea 76
- downstream helper-lane consumer clobber while argument homes are still being
  copied into the wrong places before the call; that remains in idea 75
- genuine post-link variadic traversal defects in `llvm.va_start`, `va_list`,
  or later `myprintf` walking; those remain in idea 71
- generic non-runtime backend capability gaps that fail before link or before
  the current return-value / HFA seam becomes observable

## Completion Signal

This idea is complete when the owned post-link return-value / HFA runtime
cases no longer first fail in that seam and instead either execute correctly or
graduate into a later, better-fitting runtime leaf.
