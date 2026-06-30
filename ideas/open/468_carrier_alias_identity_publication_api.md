# Carrier Alias Identity Publication API

Status: Open
Type: Prepared-module identity publication / API boundary idea
Parent: `ideas/open/467_unsupported_carrier_alias_planner_rejection.md`
Source Evidence: `build/agent_state/467_step3_carrier_alias_acceptance/`
Owning Layer: Prepared name identity publication for synthesized carrier aliases

## Goal

Define and, if justified, implement a const-correct shared prepared-module
identity surface for synthesized carrier-alias `ValueNameId`s such as
`%t50.phi.sel0` / `%t50.phi.sel1`, so carrier-alias authority records and
later RV64/original-module consumers observe the same identity.

## Why This Exists

Idea 467 Step 3 rejected a hidden `const_cast` mutation and restored tracked
source files. A const-correct attempt exposed the actual boundary: carrier
alias acceptance must publish synthesized alias identities after semantic
candidate checks pass, but current representative paths reach collectors and
object consumers through `const PreparedBirModule&`. Publishing into a scratch
copy can improve diagnostics, but original-module consumers still see the old
name table and cannot reliably match the same synthesized carrier alias
identity.

## In Scope

- Audit callers of prepared carrier-alias evidence/authority collection and
  identify where a shared mutable publication stage can run before object-route
  consumers.
- Compare two admissible routes:
  - explicit mutable carrier-alias name publication before RV64/object
    consumers receive the prepared module;
  - a consumer-facing authority record/API that does not require inserting
    scratch-copy alias names into the original prepared name table.
- Define a const-correct API contract that rejects hidden mutation and keeps
  all consumers on one coherent identity surface.
- If bounded, implement only the shared identity/API prerequisite with focused
  tests and route back to idea 467 for planner acceptance.

## Out Of Scope

- Hidden mutation through `const_cast` or other writes behind
  `const PreparedNameTables&`.
- Scratch-copy-only publication claimed as sufficient for original-module
  RV64 consumers.
- RV64 ULE rematerialization or object lowering changes.
- Accepting carrier aliases from raw `.phi.sel` spelling, raw select shape,
  value ids, block labels, function names, testcase names, or dump order.
- Plain `%t46 -> %t50` copies, generic move-bundle support, stale stack-load
  consumption, expectation rewrites, unsupported downgrades, allowlists,
  pass/fail accounting changes, or baseline/log churn.

## Acceptance Criteria

- The caller/API boundary is classified with one selected route: mutable
  pre-consumer publication stage, consumer-facing identity-free authority API,
  or a precise blocker.
- Any implementation provides one coherent identity surface to both prepared
  authority publication and later original-module consumers.
- Tests prove that synthesized carrier aliases are not published through
  hidden const mutation or scratch-copy-only state.
- Fresh residual disposition states whether idea 467 can resume its
  `unsupported_carrier_alias` planner repair.

## Reviewer Reject Signals

- Reject `const_cast` or equivalent hidden mutation of prepared name tables.
- Reject scratch-copy-only publication presented as sufficient for RV64 or
  other original-module consumers.
- Reject RV64 lowering changes, raw `.phi.sel` alias inference, plain
  successor-defined copies, generic move-bundle support, stale stack-load
  consumption, expectation rewrites, unsupported downgrades, allowlists, or
  baseline/log churn.
- Reject broad prepared-module rewrites that do not settle the carrier-alias
  identity publication boundary.
