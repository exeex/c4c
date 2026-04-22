# Prepared Local-Slot Handoff Consumption For X86 Backend

Status: Open
Created: 2026-04-22
Last-Updated: 2026-04-22
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Make the x86 prepared emitter consume authoritative prepared local-slot helper
and continuation handoff facts as real backend capability instead of rejecting
multi-block helper routes once the case has already cleared semantic lowering,
prepared-module traversal, and the earlier scalar matcher leaves.

This leaf exists because some cases no longer stop in scalar selection or
prepared-module ownership. They now fail later when `prepared_local_slot_render`
still requires an authoritative local-slot handoff before it will render the
current helper/control-flow route.

## Owned Failure Family

This idea owns backend failures that report:

- `error: x86 backend emitter requires the authoritative prepared local-slot ... handoff through the canonical prepared-module handoff`

when the route is not better explained by an already-open narrower leaf for:

- missing short-circuit or guard-chain handoff data
- scalar expression or terminator selection
- prepared call-bundle or multi-function prepared-module consumption

## Current Known Failed Cases It Owns

- `c_testsuite_x86_backend_src_00204_c`

As additional backend cases are confirmed to stop on the same authoritative
prepared local-slot handoff family, they should route here instead of being
forced back into idea 59, 60, or 61.

## Latest Durable Note

As of 2026-04-22, the fresh probe
`ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_x86_backend_src_00204_c|backend_cli_trace_mir_00204_match_rejection)$'`
showed that `backend_cli_trace_mir_00204_match_rejection` still passes and the
focused `match` helper still exposes an internal scalar restriction on the
`local-slot-guard-chain` lane, but the full top-level
`c_testsuite_x86_backend_src_00204_c` route no longer stops there. It now
fails later with
`error: x86 backend emitter requires the authoritative prepared local-slot instruction handoff through the canonical prepared-module handoff`
from `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` when
continuation or authoritative multi-block control-flow is present. Durable
ownership therefore moves out of idea 60 and into this new local-slot handoff
leaf until the full case advances again.

## Scope Notes

Expected repair themes include:

- authoritative local-slot instruction, reload, store, or continuation
  consumption once prepared control-flow ownership is already published
- local-slot helper-family rendering in
  `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
  when multi-block control-flow or continuation labels are authoritative
- explicit routing for cases that advance out of local-slot handoff and into a
  later scalar-helper, call/helper-family, or runtime leaf

## Recommended Repair Route

The intended fix direction for this idea is contract consumption over the
prepared local-slot/control-flow surfaces, not one more helper-shaped x86 fast
path for `00204.c`.

The route should be:

1. identify the authoritative local-slot and control-flow facts already
   published for the failing helper route
2. build a normalized local-slot helper plan from those prepared facts
3. make x86 render from that normalized plan
4. reject reopening raw carrier blocks, local topology recovery, or testcase
   matching once authoritative prepared ownership is present

The default assumption for this idea should be:

- if x86 has authoritative prepared control-flow blocks or continuation labels
  but still re-derives local-slot meaning from raw block shape, the contract is
  not being consumed generally enough
- if the current contract is not expressive enough for a generic local-slot
  helper plan, extend the shared prepared contract first instead of adding one
  more helper-specific matcher

## Current Contract Surface To Consume

This idea should primarily consume the existing prepared surfaces for:

- `PreparedControlFlowFunction.blocks`
- `PreparedControlFlowFunction.branch_conditions`
- `PreparedControlFlowFunction.join_transfers`
- authoritative prepared continuation labels and related local-slot helper
  metadata already passed into `prepared_local_slot_render.cpp`
- `PreparedValueLocations` and any prepared move/home data needed by the local-slot helper

Packets under this idea should begin by asking whether the current failing
route can be rendered from one generic local-slot helper plan over those
surfaces. Only if the answer is no should the contract grow.

## Contract-First Extension Rule

When an owned local-slot case reveals that the current prepared local-slot or
control-flow surface is missing information, the preferred fix is to extend the
shared prepared contract and helpers before touching x86 helper matching logic.

The intended escalation order is:

1. reuse an existing prepared helper or control-flow consumer
2. add a new shared helper over existing stored fields
3. add missing fields to the prepared value/control-flow contract in
   `prealloc.hpp`
4. update the producing phase to publish those fields
5. update x86 to consume the new contract

The reverse order is out of bounds for this idea.

## Boundaries

This idea does not own:

- short-circuit or guard-chain routes that still fail because authoritative
  prepared CFG handoff data is missing entirely; those remain in idea 59
- scalar expression or terminator selection after a helper route already
  reduces to the established scalar surfaces; those remain in idea 60
- prepared call-bundle or multi-function prepared-module consumption; those
  remain in idea 61
- trace-only observability work; that remains in idea 67
- runtime correctness bugs after codegen already succeeds

## Completion Signal

This idea is complete when the owned local-slot handoff cases stop failing for
the authoritative prepared local-slot family and instead emit successfully or
graduate into a better downstream leaf.
