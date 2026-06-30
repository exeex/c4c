# Unsupported Carrier Alias Planner Rejection

Status: Closed
Type: Prepared carrier-alias planner / producer rejection analysis idea
Parent: `ideas/closed/466_representative_select_carrier_alias_authority.md`
Source Evidence: `build/agent_state/466_step4_residual_disposition/`
Owning Layer: Prepared select carrier-alias planner and producer authority

## Goal

Diagnose and, if justified, repair why the real `20010329-1` `%t46 -> %t50`
representative row collects two carrier-alias candidates but rejects them with
`status=unsupported_carrier_alias` before publishing an available
`PreparedSelectCarrierAliasAuthority` record.

## Completion Notes

Closed after the shared identity prerequisite from idea 468 completed. Idea
467 had been parked because accepting the representative carrier aliases
required durable synthesized carrier-alias `ValueNameId` publication visible to
later original-module consumers. Idea 468 implemented that publication in a
mutable pre-consumer prepared stage.

The prior rejected row:

```text
select_carrier_alias_authority function=main status=unsupported_carrier_alias predecessor=logic.rhs.end.40 successor=logic.end.41 destination=%t50 destination_value_id=21 source=%t46 source_value_id=20 source_producer=binary source_producer_block=logic.end.41 source_producer_inst=1 carrier_alias_candidates=2 carrier_aliases=0 source_use_closure=no
```

The post-468 representative row is now available with
`carrier_aliases=2` and `source_use_closure=yes`, and the representative object
route exits `0`. There is no remaining `unsupported_carrier_alias` planner
packet for this chain.

Close validation used existing canonical regression logs and `git diff --check`.

## Reviewer Reject Signals

- Reject RV64 lowering or rematerialization changes presented as
  planner/producer metadata work.
- Reject accepting carrier aliases from `%*.phi.sel*` spelling, raw select
  shape, value ids, block labels, function names, testcase names, or dump
  order instead of producer-owned facts.
- Reject treating unavailable evidence rows as RV64 authority.
- Reject plain successor-defined `%t46 -> %t50` copies, generic move-bundle
  support, stale stack-load consumption, expectation rewrites, unsupported
  downgrades, allowlists, pass/fail accounting changes, or baseline/log churn.
