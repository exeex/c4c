# RV64 Global Residual Runtime Mismatch Research Runbook

Status: Active
Source Idea: ideas/open/642_rv64_global_residual_runtime_mismatch_research.md

## Purpose

Turn idea 642 into a research-only execution route for the residual
`src/pr79737-2.c` runtime mismatch that remains after direct global-symbol
local-memory policy has been ruled out.

## Goal

Classify the first concrete owner of the `src/pr79737-2.c` runtime mismatch
with fresh object, link, runtime, and prior-owner evidence, then publish a
focused research artifact and any needed follow-up recommendation.

## Core Rule

Do not implement a runtime fix in this idea. The only acceptable progress is
evidence-backed ownership classification and a narrow follow-up route, with no
expectation, unsupported-marker, allowlist, timeout, accounting, or runtime
comparison changes.

## Read First

- `ideas/open/642_rv64_global_residual_runtime_mismatch_research.md`
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
- `ideas/closed/618_runtime_mismatch_ownership_investigation.md`
- `docs/runtime_mismatch_ownership/index.md`
- `docs/runtime_mismatch_ownership/01_runtime_symptom_map.md`
- `docs/runtime_mismatch_ownership/02_likely_first_owner_map.md`
- `docs/runtime_mismatch_ownership/03_followup_implementation_queue.md`

## Current Targets

- Primary row: `src/pr79737-2.c`
- Output artifact: `docs/runtime_mismatch_ownership/04_global_residual_runtime_mismatch.md`
- Index update: `docs/runtime_mismatch_ownership/index.md`
- Optional lifecycle follow-up: one focused `ideas/open/*.md` implementation
  idea only if the research proves a single owner and the supervisor delegates
  that lifecycle step.

## Non-Goals

- Do not reopen direct global-symbol local-memory support from idea 631 unless
  fresh evidence proves a missing direct global-symbol authority fact before
  object emission.
- Do not implement ABI, layout, memory, relocation, call-lowering,
  branch/control-flow, or runtime support changes in this research idea.
- Do not group runtime mismatches under generic runtime support.
- Do not change tests, expectations, unsupported markers, allowlists,
  timeouts, runtime comparison behavior, or pass/fail accounting.
- Do not use `src/pr79737-2.c`-specific implementation shortcuts.

## Working Model

`src/pr79737-2.c` already moved past the direct local-memory object-emission
blocker. The next packet must rerun the row and inspect artifacts far enough to
distinguish bad global/local memory lowering, object relocation, stack layout,
ABI or call setup, branch/control flow, true runtime support, or unresolved
evidence. If the evidence proves a single implementation owner, this runbook
should document that owner and recommend a separate implementation idea rather
than changing code.

## Execution Rules

- Keep source-idea intent stable; execution findings belong in `todo.md` and
  the research artifact.
- Preserve every claim with concrete logs, object/link/runtime artifacts, code
  surfaces, or prior research references.
- Treat a runtime mismatch symptom as insufficient by itself; classify the
  first owner only after checking plausible producer and consumer boundaries.
- Keep implementation files untouched.
- Code-free research proof may use fresh build and targeted rerun logs; any
  later code-changing idea needs its own plan and proof surface.
- If evidence is incomplete, classify the row as unresolved with explicit
  missing artifacts instead of forcing an owner.

## Step 1: Refresh The Residual Row

Goal: reproduce `src/pr79737-2.c` from the current tree and capture the first
observable failure boundary.

Concrete actions:

- Run a fresh build and targeted backend/torture rerun for `src/pr79737-2.c`
  using the supervisor-selected command.
- Capture compile, object, link, and runtime outputs that explain how the row
  reaches runtime mismatch.
- Record the current symptom, exit status, first failing predicate or fault
  surface when available, and the exact artifact paths in `todo.md`.

Completion check:

- `todo.md` records the fresh command, result, artifact paths, and current
  first observable failure for `src/pr79737-2.c`.

## Step 2: Trace Candidate Owners

Goal: distinguish the first owner from nearby runtime mismatch lanes.

Concrete actions:

- Inspect generated object/code and runtime artifacts for global/local memory
  lowering, object relocation, stack layout, ABI or call setup, and
  branch/control-flow evidence.
- Compare the row against the owner lanes documented by idea 618.
- Record which candidate owners are ruled in, ruled out, or still unresolved,
  with concrete evidence for each decision.

Completion check:

- `todo.md` names the strongest first-owner classification or the exact
  missing evidence that prevents classification.

## Step 3: Publish The Research Artifact

Goal: add the durable research answer without changing implementation behavior.

Concrete actions:

- Create `docs/runtime_mismatch_ownership/04_global_residual_runtime_mismatch.md`.
- Update `docs/runtime_mismatch_ownership/index.md` to link the new answer.
- Include the rerun command, artifact paths, candidate-owner table, final
  classification, and any unresolved evidence gaps.
- If a single implementation owner is proven, recommend one focused follow-up
  idea by owner and proof surface; otherwise state why no implementation idea
  is ready.

Completion check:

- The docs answer directly classifies `src/pr79737-2.c` or marks it unresolved
  with missing evidence, and no implementation, expectation, unsupported,
  allowlist, timeout, runtime comparison, or accounting files changed.

## Step 4: Final Lifecycle Review

Goal: decide whether idea 642 is complete, needs a follow-up implementation
idea, or needs more research.

Concrete actions:

- Compare the research artifact against the source idea acceptance criteria.
- Confirm any recommended implementation work is separated by a single owner
  and proof surface.
- Ask the plan owner to close, rewrite, deactivate, or split only after the
  source-idea completion state is clear.

Completion check:

- The supervisor has enough evidence to request closure or a narrowly scoped
  lifecycle follow-up without implementation work inside idea 642.
