# Representative Select Carrier Alias Authority

Status: Closed
Type: Prepared producer/probe metadata idea
Parent: `ideas/open/465_rv64_ule_select_edge_rematerialization_consumer.md`
Source Evidence: `build/agent_state/465_step4_residual_disposition/`
Owning Layer: Prepared carrier-alias production, collection, and probe visibility

## Goal

Prove, publish, expose, or diagnose the missing matching
`PreparedSelectCarrierAliasAuthorityRecords` entry for the real `20010329-1`
representative `%t46 -> %t50` select-edge route.

## Why This Exists

Idea 465 implemented a valid RV64 consumer for explicit carrier-alias authority
records, but the representative still fails at the same
`pre_terminator_copies` coordinate. Fresh prepared output shows the structural
inputs `%t46 = bir.ule ptr %t42, %t45`, duplicate carriers
`%t50.phi.sel0` / `%t50.phi.sel1`, join transfer `%t50`, edge
`logic.rhs.end.40 -> logic.end.41`, and GPR operand homes for `%t42` / `%t45`.
No printed `carrier_alias` / `select_carrier` record appears, so the next owner
is the prepared authority production/collection/probe path, not generic RV64
move support.

## In Scope

- Audit `collect_prepared_select_carrier_alias_authorities` and related
  planner records against the real `20010329-1` prepared module.
- Determine whether the matching authority record is not produced, is produced
  but not printed/probe-visible, or is produced with fields that the RV64
  consumer does not match.
- If bounded, repair producer/collector/printer evidence so the representative
  publishes or exposes a durable authority record for `%t46 -> %t50`.
- If the record exists and the RV64 consumer matcher is missing a specific
  field, route that exact target-consumer packet back to idea 465.
- Preserve fail-closed behavior for raw `.phi.sel` inference, mismatched
  carrier/final result, wrong edge, wrong source producer, extra non-carrier
  uses, stale stack-loads, and generic move cases.

## Out Of Scope

- RV64 ULE rematerialization changes before representative authority is proven
  present and matchable.
- Inferring carrier aliases from `%*.phi.sel*` spelling, raw select shape,
  value ids, block labels, function names, testcase names, or dump order.
- Plain `%t46 -> %t50` copies or same-register no-ops for successor-defined
  `%t46`.
- Generic stack-to-register, register-to-register, or all-purpose move-bundle
  support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Completion Notes

Closed after Step 4 residual disposition. Idea 466 answered the
missing/hidden/mismatched evidence question for the representative:

- the prepared dump now contains a `select_carrier_alias_authority` evidence
  row for the real `%t46 -> %t50` route;
- the row is visible, not hidden;
- the row is rejected before authority publication with
  `status=unsupported_carrier_alias`;
- therefore RV64 has no available authority record to consume and no
  field-mismatch route yet.

Representative row:

```text
select_carrier_alias_authority function=main status=unsupported_carrier_alias predecessor=logic.rhs.end.40 successor=logic.end.41 destination=%t50 destination_value_id=21 source=%t46 source_value_id=20 source_producer=binary source_producer_block=logic.end.41 source_producer_inst=1 carrier_alias_candidates=2 carrier_aliases=0 source_use_closure=no
```

Follow-up source idea:
`ideas/open/467_unsupported_carrier_alias_planner_rejection.md`.

Do not route back to idea 465/RV64 consumer work until an available
carrier-alias authority record exists or a later packet proves an RV64 matcher
field mismatch against an available record.

Close validation used existing canonical regression logs and `git diff --check`;
no implementation, test, review, or baseline-log files were changed by this
lifecycle transition.

## Reviewer Reject Signals

- Reject RV64 rematerialization changes presented as producer/probe authority
  work.
- Reject alias authority inferred from `.phi.sel` spelling, raw select shape,
  value ids, block labels, function names, testcase names, or dump order.
- Reject plain successor-defined `%t46 -> %t50` copies, generic move-bundle
  support, stale stack-load consumption, expectation rewrites, unsupported
  downgrades, allowlists, pass/fail accounting changes, or baseline/log churn.
- Reject classification-only changes presented as capability progress unless
  the lifecycle state records the exact blocker and next owner.
