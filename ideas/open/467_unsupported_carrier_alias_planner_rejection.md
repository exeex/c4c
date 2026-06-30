# Unsupported Carrier Alias Planner Rejection

Status: Open
Type: Prepared carrier-alias planner / producer rejection analysis idea
Parent: `ideas/closed/466_representative_select_carrier_alias_authority.md`
Source Evidence: `build/agent_state/466_step4_residual_disposition/`
Owning Layer: Prepared select carrier-alias planner and producer authority

## Goal

Diagnose and, if justified, repair why the real `20010329-1` `%t46 -> %t50`
representative row collects two carrier-alias candidates but rejects them with
`status=unsupported_carrier_alias` before publishing an available
`PreparedSelectCarrierAliasAuthority` record.

## Why This Exists

Idea 466 proved the row is not hidden and is not an RV64 field mismatch. The
prepared dump now prints:

```text
select_carrier_alias_authority function=main status=unsupported_carrier_alias predecessor=logic.rhs.end.40 successor=logic.end.41 destination=%t50 destination_value_id=21 source=%t46 source_value_id=20 source_producer=binary source_producer_block=logic.end.41 source_producer_inst=1 carrier_alias_candidates=2 carrier_aliases=0 source_use_closure=no
```

Unavailable evidence rows are diagnostics only, not RV64 authority. The next
owner is the prepared carrier-alias planner/producer rejection, specifically
why two candidates for `%t50.phi.sel0` / `%t50.phi.sel1` do not become
authorized carrier aliases for final `%t50`.

## In Scope

- Audit the prepared carrier-alias planner path that classifies
  `unsupported_carrier_alias` for the representative `%t46 -> %t50` row.
- Determine why `carrier_alias_candidates=2` becomes `carrier_aliases=0` and
  `source_use_closure=no`.
- Define the semantic conditions under which duplicate carrier candidates are
  valid aliases of the same final join-transfer result.
- If sound, repair the planner/producer authority so this representative shape
  publishes an available authority record with focused positive and
  fail-closed coverage.
- If not sound, record the exact semantic blocker and route the next owner.

## Out Of Scope

- RV64 ULE rematerialization changes before the representative has an
  available authority record.
- Treating unavailable evidence rows as authority.
- Inferring aliases from `%*.phi.sel*` spelling, raw select shape, value ids,
  block labels, function names, testcase names, or dump order.
- Plain `%t46 -> %t50` copies or same-register no-ops for successor-defined
  `%t46`.
- Generic stack-to-register, register-to-register, or all-purpose move-bundle
  support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- The `unsupported_carrier_alias` rejection is classified with a precise
  planner/producer reason.
- Any implementation publishes available authority only from producer-owned
  join-transfer/source-producer facts and preserves fail-closed behavior for
  wrong final result, wrong edge, wrong source, extra non-carrier uses,
  raw-name-only shape, and unrelated rows.
- Focused coverage proves the representative-style accepted or rejected path.
- Fresh residual disposition states whether idea 465 may resume as an RV64
  consumer, whether another prepared metadata owner remains, or whether this
  source idea is complete.

## Reviewer Reject Signals

- Reject RV64 lowering or rematerialization changes presented as
  planner/producer metadata work.
- Reject accepting carrier aliases from `%*.phi.sel*` spelling, raw select
  shape, value ids, block labels, function names, testcase names, or dump
  order instead of producer-owned facts.
- Reject treating `status=unsupported_carrier_alias` evidence rows as RV64
  authority.
- Reject plain successor-defined `%t46 -> %t50` copies, generic move-bundle
  support, stale stack-load consumption, expectation rewrites, unsupported
  downgrades, allowlists, pass/fail accounting changes, or baseline/log churn.
