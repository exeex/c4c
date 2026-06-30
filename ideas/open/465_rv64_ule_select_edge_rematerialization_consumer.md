# RV64 ULE Select-Edge Rematerialization Consumer

Status: Open (parked pending representative carrier-alias authority)
Type: RV64 target-consumer idea
Parent: `ideas/closed/464_select_carrier_alias_metadata.md`
Source Evidence: `build/agent_state/464_step4_residual_disposition/`
Owning Layer: RV64 object-route consumer for select-edge ULE source producers

## Goal

Consume `PreparedSelectCarrierAliasAuthorityRecords` to determine whether RV64
can rematerialize `%t46 = bir.ule ptr %t42, %t45` at predecessor edge
`logic.rhs.end.40 -> logic.end.41` for the `%t46 -> %t50` join-transfer
publication, without raw `.phi.sel` alias inference or plain successor-defined
value copying.

## Why This Exists

Idea 464 completed the producer/prepared metadata prerequisite for duplicate
select carrier aliases. The remaining `20010329-1` route can return to an RV64
consumer only if it explicitly consumes the new carrier-alias authority
records. Earlier ideas rejected plain `%t46 -> %t50` copies because `%t46` is
successor-defined, and idea 463 rejected RV64 ULE rematerialization until
duplicate carrier aliases were producer-authorized.

## In Scope

- Audit how `PreparedSelectCarrierAliasAuthorityRecords` are exposed to the
  RV64 object route and how they relate to the coordinate-bearing
  `pre_terminator_copies` event.
- Define a target-consumer contract for unsigned pointer `ule`
  rematerialization using explicit carrier-alias authority, edge identity,
  source producer, destination join transfer, and operand availability.
- If sound, implement the narrow RV64 rematerialization packet for the
  authorized `%t46 = bir.ule ptr %t42, %t45` source producer with focused
  positive and fail-closed coverage.
- Re-probe the representative object route and route any next first-owner
  separately.

## Out Of Scope

- Inferring carrier aliases from `%*.phi.sel*` spelling, raw select shape,
  value ids, block labels, function names, testcase names, or dump order.
- Plain `%t46 -> %t50` copies or same-register no-ops for successor-defined
  `%t46`.
- Generic stack-to-register, register-to-register, or all-purpose move-bundle
  support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Reopening ideas 456, 458, 459, 460, 461, 462, 463, or 464 without new
  coordinate-bearing evidence.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- The RV64 route consumes explicit `PreparedSelectCarrierAliasAuthorityRecords`
  for the selected edge and source producer; raw alias inference remains
  rejected.
- The consumer contract proves `%t42` and `%t45` are target-consumable at the
  predecessor edge before emitting the unsigned pointer comparison.
- Positive coverage proves the authorized ULE rematerialization packet.
- Negative coverage rejects missing alias authority, wrong edge, wrong source
  producer, mismatched carrier/final result, non-consumable operands, plain
  successor-defined copies, stale stack-load authority, and generic move cases.
- Fresh residual disposition records whether `20010329-1` advanced and routes
  any next first-owner without broadening this idea.

## Lifecycle Note

Step 3 implemented and covered the RV64 consumer for explicit
`PreparedSelectCarrierAliasAuthorityRecords`, but Step 4 found the real
`20010329-1` representative still fails at the same `pre_terminator_copies`
coordinate. The current first owner is not another RV64 consumer change; it is
proving, publishing, exposing, or diagnosing a matching carrier-alias authority
record for the representative.

Active follow-up:
`ideas/open/466_representative_select_carrier_alias_authority.md`.

Do not reactivate this RV64 consumer idea until the representative publishes or
otherwise exposes a matching authority record, or until the follow-up proves
that the existing record is present and the RV64 matcher is missing a specific
field.

## Reviewer Reject Signals

- Reject any implementation that infers carrier aliases from `.phi.sel` names,
  raw select shape, value ids, block labels, function names, testcase names, or
  dump order instead of consuming `PreparedSelectCarrierAliasAuthorityRecords`.
- Reject a plain `%t46 -> %t50` copy or same-register no-op for the
  successor-defined `%t46`.
- Reject generic move-bundle support claimed as progress for this narrow ULE
  source-producer consumer.
- Reject stale stack-load consumption, expectation rewrites, unsupported-marker
  downgrades, allowlists, pass/fail accounting changes, baseline/log churn, or
  classification-only changes presented as RV64 consumer progress.
