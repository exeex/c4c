# Remaining LIR Aggregate-Owner Rejection Decomposition Runbook

Status: Active
Source Idea: ideas/open/836_lir_remaining_aggregate_owner_rejection_decomposition_blocker.md

## Purpose

Resolve the remaining LIR aggregate-owner residual families exposed by 831's
rejected comparable full-suite gate, or split them into ordered first-owner
successors when they do not share one concrete native relation.

## Goal

Identify the current first owning contract for the structured-owner-key,
matching-module-owner, and no-owner compatibility groups, then repair only the
smallest evidenced native LIR relation needed to return bounded proof to 831.

## Core Rule

Do not hide owner failures with rendered text, tag fallbacks, test filters, or
weaker diagnostics. Preserve invalid, foreign, wrong-namespace, and genuinely
ownerless rejection while repairing only a demonstrated valid-owner lookup
failure.

## Read First

- `ideas/open/836_lir_remaining_aggregate_owner_rejection_decomposition_blocker.md`
- `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`
- `ideas/closed/837_lir_nominal_type_family_architecture.md`
- `ideas/closed/834_lir_owned_type_spec_module_owner_canonicalization_blocker.md`
- Current LIR aggregate-owner verifier and HIR-to-LIR aggregate owner lookup
  paths before editing

## Current Targets And Scope

- Resume after the closed 837 architecture initiative.
- Reproduce current evidence first; do not assume the old 3026/3038 candidate
  still describes present behavior.
- Separate structured-key, matching-module-owner, and no-owner compatibility
  groups by first owning layer.
- Implement one native LIR relation only if current evidence proves a shared
  owning seam; otherwise create or request ordered successor ideas instead of
  mixing repairs.

## Non-Goals

- Do not perform 831's comparable-baseline acceptance.
- Do not edit 830 direct-call work, 829 body-parameter authority, Raw-BIR, or
  generic call/importer code.
- Do not reopen closed 832, 833, 834, or 837 without direct current
  first-owner evidence.
- Do not change expectations, unsupported markers, allowlists, filters,
  harness behavior, or rendered-text matching.

## Working Model

The old rejected 831 gate is historical evidence only. The first packet must
collect current diagnostics, classify each residual owner failure, and decide
whether one native relation owns the repair. Implementation is allowed only
after the owner boundary is explicit.

## Execution Rules

- Keep diagnosis and implementation separate unless the current evidence is
  already narrowly conclusive.
- If the families do not share one concrete seam, stop and route lifecycle to
  ordered successor ideas.
- If one bounded relation is selected, add nearby malformed and multi-path
  proof before returning to 831.
- Completion returns only to 831 Step 4 for its unchanged comparable
  full-suite gate; this route never claims baseline clearance by focused proof.

## Steps

### Step 1 - Decompose The Remaining Aggregate-Owner Rejection Families

Goal: reproduce current evidence and classify the residual groups by first
owning layer.

Primary target: aggregate-owner lookup, native LIR verification diagnostics,
and the representative failing tests from 831's rejected gate.

Actions:

- Re-run a focused current reproduction for the historical structured-key,
  matching-module-owner, and no-owner compatibility groups.
- Record whether each group still reproduces after the closed 837 route.
- Identify the first owning layer for each reproduced group.
- Decide whether the reproduced groups share one concrete native relation or
  require ordered separately scoped successors.

Completion check:

- `todo.md` states the current reproduction result, first-owner classification,
  and whether Step 2 is a bounded repair packet or lifecycle successor split.

### Step 2 - Repair One Evidenced Native Relation Or Split Successors

Goal: implement only the selected native relation, or defer to ordered
successors if no single seam owns the remaining families.

Primary target: the native LIR aggregate-owner construction/verifier seam
identified in Step 1.

Actions:

- For one shared seam, implement the smallest producer/verifier repair.
- Preserve malformed, foreign, wrong-namespace, and ownerless rejection.
- Add nearby coverage from more than one affected path.
- If no shared seam exists, do not edit code; create or request the ordered
  source ideas needed to repair each family independently.

Completion check:

- Either a focused implementation proof passes for the selected bounded
  relation, or lifecycle state names the ordered successors and parent return
  point.

### Step 3 - Prove Bounded Repair And Return To 831

Goal: provide accepted bounded evidence for 831 to rerun its unchanged Step 4
comparable full-suite gate.

Primary target: the focused multi-path tests selected by Step 2 plus a fresh
build.

Actions:

- Run a fresh build.
- Run focused multi-path proof covering the selected relation and malformed
  boundaries.
- Run a matching before/after regression guard for the focused proof when code
  changed.
- Record exactly what was repaired and what remains for 831's comparable gate.

Completion check:

- Supervisor accepts the bounded proof and lifecycle can return to
  `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`
  at unchanged Step 4. Do not claim 831 baseline clearance here.
