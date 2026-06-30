# Carrier Alias Identity Publication API

Status: Closed
Type: Prepared-module identity publication / API boundary idea
Parent: `ideas/open/467_unsupported_carrier_alias_planner_rejection.md`
Source Evidence: `build/agent_state/467_step3_carrier_alias_acceptance/`
Owning Layer: Prepared name identity publication for synthesized carrier aliases

## Goal

Define and, if justified, implement a const-correct shared prepared-module
identity surface for synthesized carrier-alias `ValueNameId`s such as
`%t50.phi.sel0` / `%t50.phi.sel1`, so carrier-alias authority records and
later RV64/original-module consumers observe the same identity.

## Completion Notes

Closed after Step 4 residual disposition. Step 3 implemented the selected
mutable pre-consumer publication stage:

- added `populate_select_carrier_alias_identity(PreparedBirModule&)`;
- called it from `BirPreAlloc::publish_contract_plans()` before const prepared
  consumers observe the module;
- interned only semantically validated synthesized carrier-alias result names
  into canonical `prepared.names.value_names`;
- preserved the boundary that identity publication is not authority and does
  not perform source-use closure;
- kept raw `.phi.sel` spelling inference, hidden `const_cast` mutation, and
  scratch-copy-only publication rejected.

Representative result from `build/agent_state/468_step3_shared_identity_publication/`:

- `20010329-1` prepared status: `0`;
- `20010329-1` object status: `0`;
- `%t46 -> %t50` now prints available carrier-alias authority with
  `carrier_aliases=2` and `source_use_closure=yes`.

This resolves the API prerequisite that parked idea 467. No remaining
identity/API packet is known.

## Follow-Up Disposition

The parked chain was dispositioned during this lifecycle review:

- idea 467 closed because the `unsupported_carrier_alias` planner rejection is
  resolved by the shared identity publication stage;
- idea 465 closed because the RV64 consumer can now consume available
  representative authority and the representative object route passes.

Close validation used existing canonical regression logs and `git diff --check`.

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
