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

- `c_testsuite_x86_backend_src_00204_c`

## Latest Durable Note

As of 2026-04-22, idea 75 step `2.2` proved the first remaining
aggregate-string / mixed-aggregate corruption for `00204.c` is not a bounded
helper-fragment defect. Read-only inspection showed the helper lane in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` consumes byval
payload addresses through `PreparedModuleLocalSlotLayout`, `PreparedValueHome`,
and `PreparedStackLayout`. The generated
`build/c_testsuite_x86_backend/src/00204.c.s` already shows overlapping homes
before the helper call: `s9` byte 8 writes `rsp+6176` while `s10` starts at
`rsp+6176`, then the same overlap repeats at `rsp+6184`, `rsp+6192`, and
`rsp+6200`. The helper then passes those published addresses directly to `fa1`
and `fa2`. Open ideas 61 and 68 do not fit this seam because the case already
cleared prepared-module traversal and local-slot handoff consumption; the next
executable route is upstream prepared byval-home publication/layout repair.

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
- later post-link variadic runtime traversal defects; those remain in idea 71

## Completion Signal

This idea is complete when the owned prepared byval-home publication/layout
cases no longer publish overlapping helper payload homes at their current first
bad seam and instead either advance into truthful downstream helper/runtime
execution or graduate into a later, better-fitting leaf.
