# Move-Bundle Evidence Gap Reconstruction Runbook

Status: Active
Source Idea: ideas/open/553_move_bundle_target_shape_evidence_gap_src_960209_1.md

## Purpose

Reconstruct enough row-level evidence for `src/960209-1.c` to classify its
current move-bundle target-shape failure without guessing first ownership.

## Goal

Produce an auditable evidence artifact for the single `evidence_gap` row and
route it to exactly one owner: RV64 materialization, prepared authority, BIR
producer, F128 quarantine, or a narrower remaining evidence gap.

## Core Rule

Do not assign ownership from the filename, C source shape, raw BIR shape,
expected register spelling, or bucket membership alone. Classification must be
based on emitted facts: event kind, phase, authority, move coordinates, value
ids, source and destination homes, scalar types, and F128 screening evidence.

## Read First

- `ideas/open/553_move_bundle_target_shape_evidence_gap_src_960209_1.md`
- `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv`
- `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.md`
- `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_representatives.tsv`
- `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification_rules.md`
- `ideas/closed/544_rv64_move_bundle_target_shape_bucket_split.md`
- `ideas/closed/551_rv64_move_bundle_materialization_from_classified_bucket.md`
- `ideas/closed/552_prepared_move_bundle_target_shape_authority_gaps.md`

## Current Scope

- Lane filter: `first_owner_lane=evidence_gap`
- Current row count: 1
- Current case: `src/960209-1.c`
- Current diagnostic key:
  `unsupported_move_bundle_target_shape;prepared_move;bundle_requires_unsupported_RV64_moves`
- Current missing fact:
  `missing_event_phase_authority_coordinate_and_move_facts`

## Non-Goals

- Do not implement RV64 lowering until ownership is known.
- Do not repair prepared or BIR behavior from testcase shape alone.
- Do not route to F128 quarantine without row-level F128 evidence.
- Do not change gcc_torture expectations, unsupported markers, allowlists, or
  runtime comparison behavior.
- Do not combine this row with larger RV64 materialization or prepared
  authority queues before its evidence is auditable.

## Working Model

- Treat this as an evidence reconstruction packet first, not an implementation
  packet.
- The existing classification proves only that the row belongs to the
  move-bundle target-shape bucket and lacks enough details for ownership.
- The useful output is a durable evidence artifact plus a clear lifecycle
  recommendation, not necessarily a passing testcase.
- If instrumentation is required, keep it diagnostic-focused and remove or
  isolate anything that is not intended as permanent compiler behavior.

## Execution Rules

- Keep routine packet results and proof commands in `todo.md`.
- Prefer a one-row allowlist for `src/960209-1.c`.
- Any code-changing diagnostic packet needs a fresh build proof and focused
  row proof.
- Reject progress that only renames diagnostics or still omits the missing
  facts named by this runbook.
- If classification yields a new implementation owner, create or update the
  correct lifecycle idea through the plan owner instead of expanding this
  runbook silently.

## Steps

### Step 1: Reproduce The Evidence Gap

Goal: confirm the current `src/960209-1.c` failure and preserve the exact
missing-fact baseline.

Actions:

- Build a one-row allowlist for `src/960209-1.c`.
- Run the delegated focused backend scan and capture the current diagnostic.
- Record the relevant log path, diagnostic text, and which required facts are
  still absent.
- Compare the current result with the classification TSV and representatives
  table so stale evidence is not used.

Completion check:

- `todo.md` records the current diagnostic, proof command, log location, and
  baseline list of missing facts.
- No implementation files are changed in this step.

### Step 2: Locate The Missing Diagnostic Authority

Goal: identify where move-bundle target-shape diagnostics should attach event,
phase, authority, coordinate, value, home, type, and F128-screening facts.

Actions:

- Inspect the path that emits
  `unsupported_move_bundle_target_shape` for prepared move bundles.
- Trace the available prepared/MIR/BIR facts at the diagnostic point.
- Determine whether the missing facts already exist but are not printed, are
  dropped before the diagnostic point, or were never produced.
- Record the owning source files and the minimal diagnostic or publication
  surface needed for Step 3.

Completion check:

- `todo.md` or a docs artifact names the diagnostic authority path and whether
  the evidence gap is printing-only, fact-propagation, or producer-owned.
- No ownership classification is made unless the row-level facts are already
  sufficient.

### Step 3: Add Or Regenerate Auditable Evidence

Goal: produce the missing row-level facts without changing semantic lowering
or pass/fail accounting.

Actions:

- If facts already exist, improve the diagnostic or reconstruction artifact so
  the row shows event kind, phase, authority, coordinate, value ids, homes,
  types, and F128-screening facts.
- If facts are absent, add the narrowest durable instrumentation or
  reconstruction path that exposes why they are absent.
- Re-run the one-row proof and record the full evidence for `src/960209-1.c`.
- Avoid testcase-shaped branches or diagnostic text that only handles this
  filename.

Completion check:

- A durable docs artifact or current log records the required facts, or states
  the narrower blocker preventing them.
- Build proof and focused row proof are recorded in `test_after.log` and
  summarized in `todo.md`.

### Step 4: Classify And Route The Row

Goal: convert the evidence into a lifecycle decision without overfitting.

Actions:

- Classify `src/960209-1.c` into RV64 materialization, prepared authority,
  BIR producer, F128 quarantine, or a narrower evidence gap.
- If the owner is an existing open idea, update `todo.md` with the handoff
  recommendation for the supervisor.
- If the owner requires a new durable initiative, ask the plan owner to create
  it with reviewer reject signals.
- If this evidence-gap idea is satisfied, ask the plan owner to close it after
  regression guard requirements are met.

Completion check:

- The row has exactly one auditable owner or an explicitly narrowed remaining
  evidence blocker.
- The classification cites row-level evidence, not testcase shape.
- No expectations, unsupported markers, allowlists, or runtime comparison
  behavior were weakened.
