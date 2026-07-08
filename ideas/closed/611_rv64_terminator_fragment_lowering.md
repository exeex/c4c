# RV64 Terminator Fragment Lowering

Status: Closed
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`
- `ideas/open/615_branch_stack_source_residual_audit.md`
- `ideas/open/616_select_publication_source_wiring.md`
- `ideas/open/617_scalar_compare_publication.md`
- `ideas/open/619_bir_aggregate_global_store_handoff.md`
- `ideas/open/621_rv64_prepared_global_value_location_consumer.md`
Owning Layer: RV64/MIR consumer
Queue Order: 10
Prerequisites: prepared branch and operand authority must exist; branch stack-source residuals remain separate
Estimated Evidence Breadth: `70` unsupported terminator-fragment rows
Proof Surface: RV64 backend-object rows with prepared terminator fragments that lack target lowering

## Goal

Implement RV64 lowering for supported terminator fragments after prepared
branch operands and control-flow authority are present.

## Why This Exists

The current scan had `70` unsupported terminator-fragment rows. This was a
large target-consumer bucket, but it could not bypass branch stack-source
freshness contracts.

## In Scope

- RV64/MIR consumer lowering for supported terminator fragments.
- Guardrails for missing branch operands, stack-source freshness, or prepared
  control-flow facts.
- Proof across multiple terminator rows.

## Out Of Scope

- Prepared branch source publication.
- Move-bundle materialization, generic instruction fragments, ABI, runtime,
  expectations, unsupported markers, allowlists, timeouts, or accounting.

## Completion Notes

The active route implemented the clean RV64 terminator-consumer slice:

- `02eebc733` lowered RV64 fused integer branches.
- `f0dcdbda1` lowered RV64 fused floating branches.
- Step 4 close-readiness classified the remaining direct-object
  `unsupported_terminator_fragment` rows as mixed first-owner residuals rather
  than a clean terminator-consumer follow-up.

Residual ownership was split into existing open ideas:

- Global storage/global data: `src/ieee/20001122-1.c` and `src/991030-1.c`
  route through `ideas/open/619_bir_aggregate_global_store_handoff.md` and,
  once prepared facts are complete, may provide guard evidence for
  `ideas/open/621_rv64_prepared_global_value_location_consumer.md`.
- Join/select publication: `src/921124-1.c` and `src/920710-1.c` route through
  `ideas/open/616_select_publication_source_wiring.md` and, if refreshed
  evidence proves compare authority is first owner, through
  `ideas/open/617_scalar_compare_publication.md`.
- Stack-backed condition/freshness: the `src/921124-1.c` stack-source aspect
  remains separated under `ideas/open/615_branch_stack_source_residual_audit.md`.
- ABI, generic instruction fragments, and move-bundle work remain out of scope
  for this closed terminator route; no current Step 4 residual required new
  ABI, generic instruction, or move-bundle idea creation.

## Acceptance Criteria

- Multiple terminator-fragment rows progress through RV64 lowering.
- Rows whose first owner is branch stack-source freshness or prepared authority
  remain separated.
- The proof checks both progressing rows and still-rejected missing-authority
  rows.

Result: accepted as split-and-closed. The source idea's clean RV64 terminator
consumer scope is complete; remaining terminator-labeled rows are durable
follow-up work under their first-owner ideas.

## Close Gate

- Backend subset command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Regression guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: passed with `346` passed before and after, `0` failures before and
  after, and no new failures.

## Reviewer Reject Signals

- Reject branch lowering that infers missing operands from final layout.
- Reject named-case-only terminator handling such as a single `src/20030910-1.c`
  shape.
- Reject combining branch-source authority publication with target consumer
  lowering.
- Reject expectation or unsupported-status downgrades.
- Reject helper renames that keep the same unsupported terminator fragment.
