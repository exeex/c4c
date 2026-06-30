# Select Carrier Alias Metadata

Status: Closed
Type: Prepared control-flow/publication metadata idea
Parent: `ideas/closed/463_select_edge_ule_source_producer_rematerialization.md`
Source Evidence: `build/agent_state/463_step4_residual_disposition/`
Owning Layer: Producer/prepared carrier-alias authority for select-edge source producers

## Goal

Publish durable prepared metadata proving when duplicate select carrier values
such as `%t50.phi.sel0` and `%t50.phi.sel1` are carrier-only aliases of one
final join-transfer result `%t50`, so later RV64 source-producer
rematerialization can consume the alias relationship without raw-shape
inference.

## Why This Exists

Idea 463 classified `%t46 = bir.ule ptr %t42, %t45` as semantically plausible
for predecessor-edge rematerialization, but blocked before RV64 lowering
because prepared metadata does not currently publish duplicate carrier-alias
authority for `%t50.phi.sel0` / `%t50.phi.sel1`. RV64 must not infer that
relationship from `.phi.sel` names, value ids, block labels, raw select shape,
or dump order.

## In Scope

- Audit prepared control-flow/publication records for select-edge source
  producers and duplicate carrier values.
- Define a carrier-alias fact keyed by function, edge, join transfer, source
  value, destination value, carrier values, and source producer.
- Publish or expose the fact through prepared metadata surfaces such as
  `PreparedJoinTransfer`, `PreparedEdgeValueTransfer`,
  `PreparedJoinTransferCarrierKind`, prepared object traversal,
  select-chain lookups, and publication plans.
- Prove use closure: all source-producer uses are either the selected edge
  source or authorized carrier aliases for the same join-transfer result.
- Add focused prepared/BIR coverage for positive duplicate-carrier alias facts
  and negative fail-closed cases.

## Out Of Scope

- RV64 ULE rematerialization or any target consumer change before metadata
  exists.
- Plain `%t46 -> %t50` copies or same-register no-ops.
- Inferring alias authority from `.phi.sel` names, raw select shape, value ids,
  block labels, function names, testcase names, or prepared dump order.
- Generic stack-to-register, register-to-register, or all-purpose move-bundle
  support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Completion Notes

Closed after Step 4 residual disposition. Step 3 committed
`PreparedSelectCarrierAliasAuthority*` records, planner, availability
predicate, collector exposure, and focused metadata tests. The metadata layer
now publishes the required producer/prepared authority surface for duplicate
select carriers.

Completed facts include:

- function and predecessor/successor edge;
- join/final result;
- selected source;
- binary source-producer site;
- carrier values and optional carrier value ids;
- use closure for authorized carrier aliases;
- collector access through `collect_prepared_select_carrier_alias_authorities`.

Negative coverage keeps missing source producer, raw-name-only or mismatched
carrier result, and extra non-carrier source use fail-closed.

No exact remaining idea-464 metadata packet is identified. Later RV64 ULE
rematerialization is separate target-consumer work and may proceed only by
consuming `PreparedSelectCarrierAliasAuthorityRecords`.

Follow-up source idea:
`ideas/open/465_rv64_ule_select_edge_rematerialization_consumer.md`.

Close validation used existing canonical regression logs and `git diff --check`;
no implementation, test, review, or baseline-log files were changed by this
lifecycle transition.

## Reviewer Reject Signals

- Reject RV64 lowering or rematerialization changes presented as producer
  metadata work.
- Reject alias authority inferred from `%*.phi.sel*` spelling, raw select
  shape, value ids, block labels, function names, testcase names, or dump
  order instead of producer-owned join-transfer linkage.
- Reject broad publication-plan rewrites that do not produce a durable
  carrier-alias fact consumable by later source-producer rematerialization.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, baseline/log churn, or classification-only
  changes claimed as metadata progress.
- Reject consuming stale stack-load authority or adding generic move-bundle
  support inside this metadata idea.
