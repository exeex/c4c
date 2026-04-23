# Grouped Register Bank And Storage Authority For Prealloc

Status: Closed
Created: 2026-04-23
Last-Updated: 2026-04-23
Closed: 2026-04-23
Parent Idea: [86_full_x86_backend_contract_first_replan.md](/workspaces/c4c/ideas/open/86_full_x86_backend_contract_first_replan.md)

## Intent

Extend prealloc authority from scalar register assignment to grouped-register
resource ownership so future vector backends can describe bank, span, alias,
and storage decisions without pushing target-specific resource reasoning back
into liveness consumers or emitters.

## Owned Failure Family

This idea owns target-independent prealloc gaps where:

- liveness intervals are already sufficient as lifetime facts
- the missing capability is grouped physical-resource modeling
- values need bank-aware and span-aware assignment, clobber, spill, or storage
  publication
- a backend would otherwise need to infer contiguous register legality or alias
  overlap on its own

## Scope Notes

Expected repair themes include:

- authority for `register_group_width` and bank classification publication
- grouped candidate legality and occupied-unit alias modeling
- grouped save/clobber publication
- grouped storage/spill/reload contracts
- regalloc tests that prove width-aware occupancy and call-boundary behavior
- future-facing support for vector groups such as contiguous `m2` / `m4`
  assignment families

## Boundaries

This idea does not own:

- frontend type lowering for any one concrete vector ISA
- target-specific register naming or final instruction emission
- reworking liveness into a physical-resource phase; liveness remains a
  lifetime-only model
- scalar-only frame/stack/call authority work that does not depend on grouped
  register semantics

## Completion Signal

This idea is complete when prealloc can publish truthful bank/span/storage
authority for grouped registers, and target backends no longer need to invent
their own contiguous-register or alias reasoning beyond spelling and final
encoding.

## Closure Notes

Grouped prealloc authority now publishes bank/span/storage decisions instead of
leaving grouped legality to downstream reconstruction. The route landed grouped
authority publication in prealloc, exposed the consumer-facing x86 debug proof
surface, and finished with focused contract plus consumer confirmation from
`backend_prepare_frame_stack_call_contract` and `backend_x86_route_debug`.
