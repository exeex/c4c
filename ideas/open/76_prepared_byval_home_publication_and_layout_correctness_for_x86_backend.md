# Prepared Byval-Home Publication And Layout Correctness For X86 Backend

Status: Open
Created: 2026-04-22
Last-Updated: 2026-04-22
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Repair the upstream prepared byval-home publication/layout seam now exposed by
`00204.c`: the bounded same-module helper consumer is honoring its current
contract, but the prepared stack/object homes it receives for helper byval
payloads already overlap before the call is rendered.

## Owned Failure Family

This idea owns x86 backend failures where:

- the prepared-module route already matches and the bounded helper lane still
  follows the canonical helper-fragment contract
- the first bad fact is that authoritative prepared byval payload homes are
  published or laid out at overlapping stack offsets before the helper call is
  emitted
- the downstream helper consumer only forwards those published homes, so a
  renderer-only fix would either break protected helper-boundary coverage or
  mask the upstream publication/layout defect

## Current Known Failed Cases It Owns

- none currently confirmed; `c_testsuite_x86_backend_src_00204_c` graduated on
  2026-04-22 into a downstream post-link return-value / HFA runtime leaf after
  the overlapping byval-home publication/layout seam stopped being the first
  bad fact

## Latest Durable Note

As of 2026-04-22, commit `d61017fa` repaired the owned publication/layout seam
generically: once one slice of an address-exposed aggregate-local family
requires a fixed home, `assign_frame_slots(...)` keeps the whole slice family
in the fixed-location group so helper byval payload homes publish contiguously
instead of interleaving only their `.0` slices. Focused `00204.c` proof after
that change still fails later at runtime, but the visible `Arguments:` section
now matches through the earlier multi-aggregate byval probes and the remaining
mismatch sits in the later return-value / HFA surface, not in overlapping
helper payload homes.

The source idea's semantic completion signal is therefore satisfied: the first
bad fact moved downstream out of publication/layout ownership. A close review
on 2026-04-22 still rejected formal closure because the strongest committed
close-grade scope available in-repo,
`ctest --test-dir build -j --output-on-failure -R '^backend_'`, stayed flat at
`103` passes before and after. Idea 76 remains open only until a committed
closure-grade probe exists for the repaired publication/layout seam; active
execution has graduated into idea 77's downstream runtime leaf.

## Scope Notes

Expected repair themes include:

- authoritative publication of non-overlapping byval payload homes for
  same-module helper calls once helper routing already matches
- agreement between prepared aggregate widths, stack/object layout, and the
  published stack offsets later consumed by the bounded helper lane
- route proof that distinguishes upstream home publication/layout from later
  helper consumption or variadic runtime work

## Boundaries

This idea does not own:

- prepared-module traversal or authoritative prepared call-bundle rejection;
  those remain in idea 61
- prepared local-slot handoff rejection before the helper route matches; that
  remains in idea 68
- downstream helper-lane consumer clobber after byval homes are already
  published truthfully; that remains in idea 75
- downstream post-link return-value / HFA runtime correctness after helper
  payload homes are already published truthfully; that remains in idea 77
- later post-link variadic runtime traversal defects; those remain in idea 71

## Completion Signal

This idea is complete when the owned prepared byval-home publication/layout
cases no longer publish overlapping helper payload homes at their current first
bad seam and instead either advance into truthful downstream helper/runtime
execution or graduate into a later, better-fitting leaf.
