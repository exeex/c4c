# LIR Canonical Module-Owned Aggregate Ref/Store Convergence Runbook

Status: Active
Source Idea: ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md
Activated from: closure of 837 A1 architecture umbrella handoff

## Purpose

Implement the first nominal-family owner: stable aggregate identity from HIR
occurrences through a module-owned LIR aggregate store, without absorbing the
parked 836 route or adjacent nominal-family migrations.

## Goal

Make aggregate-bearing occurrences use a stable canonical HIR aggregate
reference and intern it exactly once into the owning LIR module's aggregate
store.

## Core Rule

Aggregate identity comes from canonical HIR facts, never tags, parser pointers,
rendered text, or reconstructed lookup keys. Preserve the three 836 failure
groups as distinct evidence; do not complete 836 or change its 831 Step 4 return.

## Read First

- `ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md`
- `docs/lir_nominal_type_family_architecture/current_lir_type_ref_responsibility_matrix.md`
- `docs/lir_nominal_type_family_architecture/nominal_family_boundary_decisions.md`
- `docs/lir_nominal_type_family_architecture/dependency_ordering.md`
- `docs/lir_nominal_type_family_architecture/closure_trace.md`
- `ideas/open/836_lir_remaining_aggregate_owner_rejection_decomposition_blocker.md`
- `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`

## Scope

- M4--M6: canonical aggregate identity, aggregate store facts, and bounded
  aggregate-bearing lowering/consumer convergence.
- `HirAggregateRef { ModuleId, HirAggregateId }` or a proven equivalent,
  `LirAggregateRef`, and module/lowering-session mapping lifetime.
- Named, anonymous, local, template, and typedef/alias aggregate forms;
  ordered fields, layout, projections, and recursive typed children.

## Non-Goals

- Do not close/resume 836 or 831, alter their return, or merge the incomplete,
  unmatched-owner, and no-owner-compatibility groups.
- Do not migrate function, vector, scalar, unions, generic verifier/printer,
  collectors, globals, or universal-model deletion work.
- Do not use runtime text, tags, parser pointers, reconstructed owner keys,
  RTTI, universal-ID facades, or testcase-shaped exceptions.

## Execution Rules

- Keep each packet buildable and record packet evidence in `todo.md`.
- Register canonical definitions before occurrence lowering; unknown,
  incomplete, stale, foreign, and wrong-module refs fail closed.
- Preserve accepted 754/798/801/803 behavior. Delete legacy `record_def`,
  owner-key, tag/text lookup, and duplicate metadata only after every named
  declaration, field, call, verifier, printer, and receiver consumer migrates.

## Ordered Steps

### Step 1 - Establish the canonical aggregate identity and store seam

Goal: identify the current HIR canonical owner/index and introduce the smallest
stable HIR-to-LIR aggregate ref/store contract.

Actions:

- Inspect aggregate definitions, occurrence lowering, owner keys, and module
  ownership boundaries.
- Add stable carrier and module-owned LIR aggregate store/ref with explicit
  registration and mapping lifetime.
- Prove repeated occurrences intern once without tag/text/key reconstruction.

Completion check: fresh build plus focused lowering proof demonstrates one
canonical definition-to-store mapping and explicit unknown/incomplete rejection.

### Step 2 - Preserve recursive aggregate facts and all aggregate forms

Goal: migrate kind, fields, layout, projections, and recursive children without
flattening semantic structure.

Actions:

- Cover named, anonymous, local, template, and typedef/alias occurrences and
  registration-before-use ordering.
- Retain legitimate no-owner rendered compatibility only at its named consumer.

Completion check: focused lowering, verifier, and printer coverage proves nested
and repeated aggregate parity without competing operation-local authority.

### Step 3 - Enforce module ownership and migrate bounded consumers

Goal: make justified consumers use canonical store facts and fail closed.

Actions:

- Migrate bounded declaration, field, call, verifier, printer, and receiver
  consumers justified by the store seam.
- Prove foreign, wrong-module, stale, and malformed rejection; retain only
  named adapters with explicit deletion gates.

Completion check: fresh build plus focused valid/invalid proof preserves
754/798/801/803 seams and rejects cross-module/foreign refs.

### Step 4 - Assess bounded convergence and hand off remaining work

Goal: verify A1's contract and document exact remaining deletion gates.

Actions:

- Confirm every introduced carrier has one owner and no text/key/tag recovery.
- Run selected focused proof and the proportional shared-surface checkpoint.
- Record remaining adapters without absorbing 836, later successors, 812/813,
  or 797.

Completion check: source acceptance has supervisor-accepted proof or this route
returns to plan-owner with exact unmet criteria.
