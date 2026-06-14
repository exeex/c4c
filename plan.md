# 254 Phase F3 Prepared Compatibility Fail-Closed Proof Matrix Runbook

Status: Active
Source Idea: ideas/open/254_phase_f3_prepared_compatibility_fail_closed_proof_matrix.md

## Purpose

Create a reusable proof matrix for prepared compatibility surfaces that must
remain stable while future route/BIR semantic facts move underneath them.

## Goal

Define the compatibility and fail-closed rows future reviewers must use to
reject expectation weakening, public-surface drift, or old-failure retention.

## Core Rule

This runbook is proof-matrix work only. Do not implement adapters, demotions,
baseline rewrites, expectation weakening, field deletion, public API
privatization, or broad prepared retirement.

## Read First

- `ideas/open/254_phase_f3_prepared_compatibility_fail_closed_proof_matrix.md`
- Current prepared compatibility helpers, status names, route-debug output,
  wrapper output, and target-policy-sensitive output before writing matrix
  rows.
- Prior route/BIR compatibility surfaces only as evidence for current public
  behavior; do not treat them as permission to move semantic authority.

## Current Targets And Scope

- Missing, invalid, duplicate/conflict, mismatch, unsupported, prepared-only,
  fallback, and policy-sensitive fail-closed rows.
- Helper/oracle statuses, fallback names, route-debug names, prepared
  printer/debug text, wrapper output, `ConsumedPlans`, source-memory statuses,
  publication statuses, and exact target output rows.
- Row grouping for common surfaces and fact-family-specific surfaces:
  calls, memory, edge publications, source producers, names, control flow, and
  store-source publications.

## Non-Goals

- Do not implement x86, riscv, route, BIR, adapter, demotion, or publication
  logic.
- Do not refresh baselines, expected strings, target output, helper names, or
  status names.
- Do not delete, privatize, rename, or broadly wrap prepared public fields or
  lookup helpers.
- Do not move target policy into BIR or route facts.
- Do not mark compatibility preservation as semantic ownership transfer.

## Working Model

- Prepared compatibility remains authoritative for observable public surfaces.
- Route/BIR agreement can be required by future implementation packets, but
  compatibility rows must keep old public behavior stable until a separate
  source idea authorizes a specific ownership transfer.
- A matrix row should identify both the public prepared surface and the
  route/BIR agreement or fail-closed behavior that future work must prove.

## Execution Rules

- Prefer a durable Markdown matrix artifact over chat-only notes.
- Keep row wording concrete enough for `c4c-reviewer` to use as a reject
  checklist.
- Mark row scope as common or fact-family-specific.
- For every compatibility surface, record the expected rejection behavior for
  missing, stale, mismatched, unsupported, prepared-only, route/BIR-only, and
  policy-sensitive evidence when applicable.
- If a row cannot be proved from current code or tests, record it as an
  explicit open proof gap instead of weakening the requirement.

## Step 1: Inventory Prepared Compatibility Surfaces

Goal: find the public prepared compatibility surfaces that future packets must
preserve.

Primary target: current helper/oracle status, fallback, route-debug, prepared
printer/debug, wrapper, `ConsumedPlans`, source-memory, publication, and target
output surfaces.

Actions:

- Inspect current prepared compatibility readers and tests for calls, memory,
  edge publications, source producers, names, control flow, and store-source
  publications.
- List each observable status name, fallback name, debug/printer text,
  wrapper row, consumed-plan row, and target-policy-sensitive output row.
- Separate common rows from rows that apply to only one fact family.

Completion check:

- The executor can name each compatibility surface that needs a matrix row and
  can point to the code or test evidence for that surface.

## Step 2: Draft The Common Fail-Closed Matrix

Goal: capture compatibility rows shared across future prepared-to-route/BIR
packets.

Primary target: a reusable matrix section for missing, invalid,
duplicate/conflict, mismatch, unsupported, prepared-only, route/BIR-only,
fallback, and policy-sensitive behavior.

Actions:

- Write common rows for public status preservation, fallback preservation,
  unsupported fail-closed behavior, and expected-string stability.
- For each row, identify the prepared surface, required agreement or rejection
  condition, and reviewer reject signal.
- Call out rows that must not be satisfied by helper renames,
  classification-only changes, expectation rewrites, or named-case shortcuts.

Completion check:

- The common matrix can be applied to more than one fact family without
  relying on a single testcase or expected string.

## Step 3: Add Fact-Family-Specific Rows

Goal: extend the matrix with concrete rows for calls, memory, edge
publications, source producers, names, control flow, and store-source
publications.

Primary target: fact-family sections that name the exact prepared surface and
  future route/BIR agreement gate.

Actions:

- Add call rows for call-plan public APIs, wrapper/fallback behavior,
  route-debug output, helper/oracle statuses, and `ConsumedPlans`.
- Add memory and source-memory rows for lookup/status preservation, fallback
  names, and route/prepared agreement failures.
- Add edge-publication rows for publication status, helper/oracle behavior,
  source producer identity, dynamic `LoadLocal` dependencies, and target
  output policy.
- Add names, control-flow, and store-source-publication rows where future
  one-reader packets must preserve printer/debug, route-debug, fallback, and
  output contracts.

Completion check:

- Each fact-family section has enough rows to reject prepared-only,
  route/BIR-only, mismatch, duplicate/conflict, unsupported, fallback, and
  policy-sensitive overfit where that condition applies.

## Step 4: Validate Matrix Usability

Goal: make the matrix usable as a future review checklist without changing
code or baselines.

Primary target: the completed matrix artifact and focused documentation proof.

Actions:

- Check the matrix against the source idea's acceptance criteria and reviewer
  reject signals.
- Verify each row identifies a public prepared compatibility surface plus the
  route/BIR agreement or fail-closed behavior required before acceptance.
- Record any open proof gaps without weakening compatibility requirements.
- Run only documentation or formatting checks that are appropriate for the
  artifact changed; do not run implementation validation as proof of this
  lifecycle/documentation packet.

Completion check:

- Future reviewers have a concrete checklist for rejecting expectation
  weakening, old-failure retention, field deletion, privatization, broad
  prepared retirement, or testcase-shaped shortcuts.
