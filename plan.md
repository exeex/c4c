# Subsystem Entropy Reduction Refactor Generator Runbook

Status: Active
Source Idea: ideas/open/subsystem-entropy-reduction-refactor-generator.md

## Purpose

Turn the umbrella entropy-reduction idea into a bounded audit run that produces
focused follow-up refactor ideas. This active plan does not implement the
refactors it discovers.

## Goal

Audit one selected subsystem, classify its entropy hotspots, and create
actionable follow-up ideas under `ideas/open/` using the source idea's
behavior-preserving refactor rules.

## Core Rule

Do not change implementation code while executing this runbook. The output is
planning state: an entropy map, a prioritized hotspot list, and small
follow-up ideas that can later be activated through the normal lifecycle.

## Read First

- `ideas/open/subsystem-entropy-reduction-refactor-generator.md`
- `AGENTS.md`
- nearby subsystem source files only after a target subsystem is selected
- local proof commands already used by recent plans, if discoverable without
  scanning `ideas/closed/`

## Current Scope

Initial candidate subsystems from the source idea:

- `src/backend/mir/aarch64/codegen`
- `src/backend/prealloc`
- `src/backend/bir`
- `src/codegen/lir`
- `src/frontend/parser`
- `src/frontend/hir`

The supervisor may narrow Step 1 to one subsystem before execution begins.

## Non-Goals

- Do not edit implementation files, tests, or expectations.
- Do not downgrade supported behavior or convert tests to unsupported.
- Do not use LLVM or another external project as the layout reference.
- Do not create broad multi-purpose refactor ideas.
- Do not combine multiple refactor types unless the coupling is small and
  explicitly justified in the generated idea.
- Do not claim capability progress from this audit; capability work belongs to
  later activated ideas.

## Working Model

Classify each inspected file or helper group by responsibility before proposing
changes:

- data contract
- phase logic
- adapter logic
- emission logic
- debug/print logic
- compatibility bridge

A hotspot is high entropy when responsibilities are mixed without a durable
owner, a broad header exposes private concepts, a thin wrapper has no stable
ownership, or legacy bridge names remain after the newer contract owns the
behavior.

## Execution Rules

- Keep source intent in the linked idea unchanged unless durable intent is
  proven wrong.
- Record routine audit progress in `todo.md`.
- When creating follow-up ideas, use small behavior-preserving scopes with
  concrete target files, refactor type, owner category, proof expectation, and
  reject signals.
- Prefer local line-count, include, symbol, and text-search tools before
  drawing conclusions.
- Treat testcase-shaped shortcuts, expectation rewrites, and unsupported-test
  conversions as blocking reject signals.

## Ordered Steps

### Step 1: Select And Baseline One Subsystem

Goal: choose the first audit target and collect low-cost shape data.

Primary target: one candidate subsystem from `Current Scope`.

Actions:

- Select one subsystem, preferring a target with visible entropy risk and
  bounded file count.
- Collect line counts with `scripts/count_src_lines.py` or `wc -l`.
- Identify the largest `.cpp` and `.hpp` files.
- Identify broad headers and obvious include fan-out.
- Search for names containing `fallback`, `compat`, `legacy`, `bridge`,
  `temporary`, `workaround`, and old route names.
- Note nearby proof commands used by existing tests or build workflows without
  running broad validation.

Completion check:

- `todo.md` names the selected subsystem and records the baseline facts needed
  for Step 2.

### Step 2: Build The Entropy Map

Goal: classify the selected subsystem by responsibility and identify ownership
mixing.

Primary target: files and helper groups inside the selected subsystem.

Actions:

- Classify each inspected file or helper group as data contract, phase logic,
  adapter logic, emission logic, debug/print logic, compatibility bridge, or a
  justified combination.
- Mark cross-family includes or repeated concept names that suggest misplaced
  ownership.
- Separate true stable contracts from helper glue, wrappers, and route
  compatibility code.
- Keep observations evidence-backed with file paths and symbol/helper names.

Completion check:

- `todo.md` contains a short entropy map with each hotspot tied to concrete
  files or helpers.

### Step 3: Prioritize Hotspots

Goal: turn the entropy map into a ranked list of low-risk follow-up targets.

Primary target: the selected subsystem's highest-signal hotspots.

Actions:

- Rank hotspots by safety, reuse value, and clarity of durable ownership.
- Assign each candidate exactly one refactor type from the source idea:
  header contraction, family merge, contract split, phase extraction, helper
  absorption, naming repair, printer alignment, or bridge retirement.
- Reject candidates that require semantic behavior changes or broad ownership
  redesign.
- Identify the expected behavior-preservation proof for each candidate.

Completion check:

- `todo.md` contains a prioritized hotspot list and explicitly excludes any
  candidate that would be too broad for one focused follow-up idea.

### Step 4: Create Follow-Up Refactor Ideas

Goal: write actionable source ideas for the best behavior-preserving cleanup
targets.

Primary target: new files under `ideas/open/`.

Actions:

- Create multiple small follow-up ideas, ordered by safety and reuse value.
- For each idea, include target files, refactor type, durable owner or
  responsibility category, expected proof, and explicit reject signals.
- Include reject signals for expectation downgrades, testcase-shaped shortcuts,
  hidden semantic changes, target-specific logic moved into generic layers,
  giant multi-purpose refactors, broad renames without durable concept proof,
  and reduced file count that increases responsibility mixing.
- Do not activate a follow-up idea unless the supervisor delegates a lifecycle
  switch.

Completion check:

- The generated ideas are present under `ideas/open/`, are small enough for one
  focused agent run, and can be reviewed without re-reading the full audit.

### Step 5: Lifecycle Handoff Or Close Decision

Goal: decide whether the umbrella generator run is complete or should hand off
to one generated idea.

Primary target: `plan.md`, `todo.md`, and the linked source idea.

Actions:

- Verify the audit produced the required entropy map, hotspot list, and
  follow-up ideas.
- If the source idea's acceptance criteria are satisfied, ask the plan owner to
  close this active plan through normal lifecycle rules.
- If one generated idea should start immediately, ask the plan owner to switch
  active lifecycle state instead of silently expanding this runbook.

Completion check:

- The active umbrella plan is either ready for closure or intentionally handed
  off through a normal lifecycle transition.

## Final Validation

- Run `git diff --check` after lifecycle edits.
- Because this runbook is planning-only until follow-up ideas are activated, no
  build or test run is required for activation.
