# Prepared Target Consumer Boundary Audit Runbook

Status: Active
Source Idea: ideas/open/418_prepared_target_consumer_boundary_audit.md

## Purpose

Audit RV64 and AArch64 prepared target consumers after the idea 413 verifier
taxonomy, then publish concrete owner classifications that ideas 414 through
417 can consume.

## Goal

Produce `docs/prepared_fact_contracts/target_consumer_boundary_audit.md` with
target-consumer findings, taxonomy row references, owner classifications, and
downstream handoff rows before the narrower typed contract ideas start.

## Core Rule

Classify target consumer behavior using the idea 413 taxonomy. Do not add
target-local inference for missing ABI homes, helper operands, materialized
values, storage layout, initializer bytes, or raw BIR expression semantics.

## Read First

- `ideas/open/418_prepared_target_consumer_boundary_audit.md`
- `docs/prepared_fact_contracts/contract_taxonomy_and_fact_family_matrix.md`
- `src/backend/prealloc/prepared_contract_verifier.hpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- RV64 and AArch64 MIR/prepared consumer surfaces under `src/backend/mir/`

## Current Targets

- RV64 and AArch64 target helpers that inspect raw BIR instructions, globals,
  type text, source spellings, fallback names, or target-local layout details.
- Prepared consumer paths that should instead consume explicit facts covered by
  the idea 413 taxonomy rows.
- The downstream handoff boundary for ideas 414, 415, 416, and 417.

## Non-Goals

- Do not rewrite all RV64 or AArch64 lowering.
- Do not mask producer defects in target code.
- Do not change default CTest selection or add RV64 gcc_torture to the default
  harness.
- Do not weaken tests, allowlists, unsupported expectations, or runtime checks.
- Do not claim capability progress through classification-only churn when the
  source idea requires a concrete cleanup or owner decision.

## Working Model

- Acceptable coherent-fact lowering: the target lowers an explicit prepared
  fact without reconstructing missing producer state.
- Target scheduling over explicit prepared facts: the target chooses an order
  or instruction form from complete facts already published by prepared
  producers.
- Producer contract gap: the target needs a fact that the prepared producer did
  not publish or published incoherently.
- Semantic BIR gap: invalid or underspecified BIR semantics reached prepared
  publication and need an upstream semantic owner, not a target workaround.

## Execution Rules

- Use stable row IDs from the taxonomy matrix when classifying findings.
- Prefer `rg` plus focused reads over broad rewrites during the audit.
- When a target-side recovery path is removed or quarantined, preserve a precise
  producer-owned diagnostic or fail-closed contract report.
- If no safe cleanup is available in the selected packet, record a concrete
  follow-up owner decision in the audit artifact instead of forcing a risky
  code change.
- Keep documentation and code changes scoped to prepared target consumer
  boundaries.
- For any code-changing step, run `cmake --build --preset default` plus the
  supervisor-delegated CTest subset. Escalate to default CTest before close.

## Step 1: Inventory Target Consumer Recovery Sites

Goal: find target helpers that recover or reconstruct facts that should be
prepared.

Actions:

- Inspect RV64 and AArch64 MIR/codegen paths for raw BIR instruction scans,
  global/object-data reads, source spelling fallback, type text checks,
  fallback names, stack-layout inference, and nearby producer-chain recovery.
- Group findings by taxonomy row and by downstream owner idea:
  call argument routes, value materialization, helper operands, storage/layout,
  publication, and global initializer facts.
- Record file references and short evidence snippets in `todo.md`, not in the
  source idea.

Completion check:

- `todo.md` lists audited helper groups, file references, suspected taxonomy
  rows, and a first-pass downstream owner for each selected finding.

## Step 2: Draft The Audit Artifact

Goal: create the target consumer audit document with row-level classifications.

Actions:

- Create `docs/prepared_fact_contracts/target_consumer_boundary_audit.md`.
- For each selected finding, record:
  - audit row ID,
  - target/helper surface,
  - consumed taxonomy row IDs,
  - owner classification,
  - current behavior,
  - required consumer behavior,
  - downstream owner idea or explicit no-follow-up decision.
- Separate acceptable coherent-fact lowering from target recovery of missing
  producer facts.

Completion check:

- The audit artifact is enough for ideas 414 through 417 to cite concrete 418
  rows instead of re-running the same search.

## Step 3: Make One Low-Risk Cleanup Or Owner Decision

Goal: satisfy the source idea's concrete cleanup/decision requirement without
expanding into a broad lowering rewrite.

Actions:

- Choose one low-risk target-side recovery path where an existing prepared fact
  already covers the required information, or one finding where the correct
  action is a precise follow-up owner decision.
- If changing code, remove or quarantine the recovery path and route failures to
  an existing producer/prepared diagnostic or verifier report.
- If documenting an owner decision instead of changing code, make the decision
  concrete enough for a downstream executor to own it without rediscovery.
- Add focused tests only when behavior changes.

Completion check:

- Either a bounded cleanup is implemented and proved, or the audit artifact
  records a concrete follow-up owner decision with file references, row IDs,
  and reject signals for downstream work.

## Step 4: Publish Downstream Handoff Rows

Goal: make 418 usable as the handoff input for ideas 414 through 417.

Actions:

- Ensure the audit artifact contains a downstream handoff section for:
  - idea 414 typed prepared call argument contracts,
  - idea 415 prepared value materialization contracts,
  - idea 416 prepared helper operand home contracts,
  - idea 417 prepared storage layout and initializer contracts.
- Cite the taxonomy row IDs and 418 audit row IDs each downstream idea should
  consume first.
- Explicitly state when a downstream idea has no applicable finding for a
  selected route.

Completion check:

- Each downstream idea can start from named taxonomy rows plus named 418 audit
  rows or an explicit no-applicable-row note.

## Step 5: Validate And Close-Readiness Review

Goal: prove the audit/cleanup did not regress normal tests and decide whether
the source idea is complete.

Actions:

- Run the delegated build and test subset after any code changes.
- Before closure, run or record supervisor-provided default
  `ctest --test-dir build -j --output-on-failure` proof.
- Check for testcase-overfit, expectation weakening, target fallback inference,
  and broad rewrites outside prepared consumer boundaries.
- Update `todo.md` with proof results and close-readiness notes.

Completion check:

- The audit artifact is published, the cleanup/owner-decision requirement is
  satisfied, normal CTest proof is available, and no reviewer reject signal is
  present.
