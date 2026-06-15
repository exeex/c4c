# Phase F4 PreparedBirModule Liveness Authority Blocker Follow-Up

Status: Active
Source Idea: ideas/open/268_phase_f4_prepared_bir_module_liveness_authority_blocker_follow_up.md

## Purpose

Map the authority, reader, compatibility surface, and fail-closed proof needed
before `PreparedBirModule::liveness` can move out of blocked public prepared
authority.

## Goal

Produce an evidence-backed blocker map that names the exact identity-only
reader and semantic liveness fact if they exist, or records that no demotion or
private-pass-context packet is currently authorized.

## Core Rule

This runbook is analysis and blocker mapping only. Do not demote, delete,
privatize, wrap, migrate, or implement `PreparedBirModule::liveness`; do not
move target-owned policy into BIR.

## Read First

- `ideas/open/268_phase_f4_prepared_bir_module_liveness_authority_blocker_follow_up.md`
- Definitions and constructors for `PreparedBirModule`
- Readers and printers of `PreparedBirModule::liveness`
- Route-debug, helper/oracle/status, fallback, wrapper, and target-output paths
  that may expose liveness-derived behavior
- BIR or route-level liveness facts that could act as semantic authority

## Current Targets

- `PreparedBirModule::liveness`
- The exact identity-only reader, if one exists
- The single semantic liveness fact that should own meaning
- Public prepared compatibility surfaces that must remain stable
- Fail-closed rows for absent/skipped, stale, mismatch, duplicate/conflict,
  unsupported, fallback, and derived printer/target behavior

## Non-Goals

- `PreparedBirModule::liveness` demotion, deletion, privatization, accessor
  migration, or wrapper creation.
- Broad `PreparedBirModule` retirement.
- Moving target register, stack, layout, storage, ABI, emission, wrapper,
  formatting, instruction spelling, or exact target-output policy into BIR.
- Rewriting printer, route-debug, helper/oracle/status, fallback, wrapper, or
  target expectations as a substitute for semantic proof.
- Claiming capability progress through expectation rewrites, helper renames,
  status/oracle relabeling, output changes, or classification-only notes.

## Working Model

- Treat public prepared `liveness` as blocked authority until one semantic fact
  and one exact identity-only reader are proven.
- A valid identity-only reader reads identity, not target policy, formatting, or
  output details.
- The retained public prepared surface is compatibility authority unless later
  proof shows it is only a mirror of the semantic fact.
- If any fail-closed row lacks supporting evidence, the correct result is a
  blocker map, not a demotion or private-pass-context proposal.

## Execution Rules

- Preserve the source idea; record packet notes and evidence in `todo.md`.
- Prefer semantic liveness authority over testcase-shaped matching or named
  fixture proof.
- For each row, name the relevant files/functions, expected fail-closed
  behavior, and compatibility surface that must stay stable.
- Stop at the blocker map if implementation eligibility remains unsafe.
- If a separate initiative appears necessary, ask the supervisor for a new
  lifecycle idea instead of expanding this runbook.

## Ordered Steps

### Step 1: Inventory Liveness Authority

Goal: identify where prepared liveness is created, stored, published, read, and
printed.

Primary target: `PreparedBirModule::liveness`.

Actions:

- Inspect definitions and construction paths for `PreparedBirModule`.
- Trace producers of prepared liveness rows.
- Trace readers of prepared liveness rows.
- List retained public prepared compatibility surfaces.
- Identify any BIR or route-level liveness fact that could own semantic meaning.

Completion check:

- The execution notes name the prepared producers, readers, compatibility
  surfaces, and candidate semantic fact without changing implementation files.

### Step 2: Locate the Identity-Only Reader

Goal: decide whether one exact identity-only reader exists.

Primary target: readers that consume liveness identity without owning target
policy.

Actions:

- Classify each liveness reader as identity-only, printer/debug/status,
  fallback, wrapper, target-policy, or unsupported behavior.
- For any candidate identity-only reader, record the exact file/function and
  the identity it consumes.
- If no exact identity-only reader exists, record why private-pass-context or
  demotion work remains unsafe.
- Tie the reader decision back to the candidate semantic liveness fact.

Completion check:

- The execution notes either name one exact identity-only reader and one
  semantic liveness fact, or record that no exact identity-only reader exists
  yet.

### Step 3: Map Fail-Closed Rows

Goal: cover every blocker row required before later authority movement can be
considered.

Primary target: liveness row validity and disagreement behavior.

Actions:

- Map absent and skipped liveness rows.
- Map stale rows.
- Map mismatch rows.
- Map duplicate and conflict rows.
- Map unsupported rows.
- Map fallback rows.
- Map derived printer and target behavior that must remain stable.
- For each row, record the expected fail-closed behavior and required
  supporting evidence.

Completion check:

- The execution notes contain a complete row map for every required row family
  and do not leave nearby same-feature cases unexamined.

### Step 4: Decide Later Work Eligibility

Goal: convert the evidence map into a lifecycle decision without doing
implementation work.

Primary target: the completed liveness blocker map.

Actions:

- If the identity-only reader, semantic fact, compatibility surface, and every
  fail-closed row are supported, record the minimum conditions for a separate
  later implementation idea.
- If any required evidence is missing, record that no private-pass-context,
  demotion, wrapper, or migration packet is authorized.
- Identify any separate initiative that must be created under `ideas/open/`
  before implementation proceeds.

Completion check:

- The execution notes state whether later work is blocked or eligible only for a
  separate lifecycle idea, and the decision is tied to row-map evidence.

### Step 5: Validate the Map

Goal: prove the runbook result is a map-only lifecycle slice with no route
drift.

Primary target: planning artifacts and gathered evidence.

Actions:

- Check that no implementation files, expectations, helper/oracle statuses,
  fallback names, route-debug output, prepared-printer output, wrapper output,
  exact target output, unsupported behavior, or baselines were weakened.
- Check that the map does not authorize broad `PreparedBirModule` retirement or
  liveness demotion.
- Ask the supervisor whether reviewer scrutiny is needed before closure or a
  follow-up implementation idea.

Completion check:

- The supervisor can decide whether to close, rewrite, or split the active
  lifecycle state from the map and proof notes.
