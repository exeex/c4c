# Representative Select Carrier Alias Authority

Status: Open
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

## Acceptance Criteria

- The representative authority path is classified as one of: missing producer
  record, present-but-not-visible record, present-but-RV64-mismatched record,
  or exact unsupported blocker.
- Any implementation publishes or exposes a producer-owned authority record for
  the real representative without raw-name or raw-shape inference.
- Focused coverage proves the representative-style authority path and
  fail-closed negative cases.
- Fresh residual disposition says whether idea 465 can resume as a target
  consumer, whether this producer/probe idea is complete, or whether a new
  owner is required.

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
