# RV64 Cast Instruction Fragment Consumers Runbook

Status: Active
Source Idea: ideas/open/623_rv64_cast_instruction_fragment_consumers.md

## Purpose

Recover RV64/MIR cast-shaped `unsupported_instruction_fragment` rows only after
fresh evidence proves the first stop is a consumer-side cast fragment with
complete upstream producer facts.

## Goal

Refresh current cast residual diagnostics, split ownership, and implement only
the cast consumer sub-family that has real breadth and complete facts.

## Core Rule

Do not implement from stale idea-612 counts or named testcase shape. Every code
packet must be grounded in refreshed cast owner evidence and must keep nearby
non-cast rows outside this route.

## Read First

- `ideas/open/623_rv64_cast_instruction_fragment_consumers.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`

## Current Targets

- Cast-shaped `unsupported_instruction_fragment` rows that remain after idea
  612 pointer-route closure.
- RV64/MIR consumer lowering only when BIR producer facts, prepared facts,
  width, signedness, source kind, and authority are already complete.
- Diagnostic-preserving rejection for rows owned by semantic cast production,
  ABI, global, local-memory, select, branch, move-bundle, runtime, policy, or
  other non-RV64-consumer layers.

## Non-Goals

- BIR semantic cast production.
- Pointer `BinaryInst`, scalar narrow-integer, move-bundle, terminator, ABI,
  runtime, local-memory, select, branch, global, expectation, unsupported
  marker, allowlist, timeout, or accounting changes.
- Floating, vector, or library policy cast lanes unless refreshed evidence
  proves ordinary-C backend leverage and this runbook is narrowed before
  implementation.

## Working Model

- Treat the first packet as evidence gathering, not implementation.
- Classify each refreshed cast row by the first missing owner fact.
- Implement only a cast sub-family with multiple refreshed rows or record a
  no-breadth blocker in `todo.md`.
- Preserve unsupported diagnostics for rows that still lack upstream facts.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Do not edit `ideas/open/623_rv64_cast_instruction_fragment_consumers.md`
  unless durable source intent changes.
- Do not downgrade supported-path expectations or weaken test contracts.
- Do not claim progress through expectation, unsupported-marker, allowlist,
  timeout, or accounting rewrites.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.
- Escalate to supervisor/reviewer if the only available path is a single
  named-case shortcut or a broad non-cast route.

## Ordered Steps

### Step 1: Refresh cast residual evidence

Goal: Replace stale idea-612 cast assumptions with current diagnostics.

Concrete actions:
- Run the supervisor-delegated diagnostic refresh for cast-shaped
  `unsupported_instruction_fragment` rows.
- Capture row identifiers, source operations, diagnostic text, and nearby
  non-cast guard rows.
- Separate cast rows from non-cast residuals before any implementation work.

Completion check:
- `todo.md` records the refresh command, proof artifact, cast row list, and
  negative guard rows.
- No implementation files are changed in this step.

### Step 2: Split ownership by first missing fact

Goal: Identify which refreshed rows are real RV64/MIR consumer gaps.

Concrete actions:
- Bucket each cast row by first missing or responsible owner: RV64 consumer,
  BIR producer, semantic cast, ABI, global, local-memory, select, branch,
  move-bundle, runtime, policy, width, signedness, source kind, or authority.
- For candidate RV64 consumer rows, verify producer facts are complete.
- Record rows that must remain unsupported or move to other owner routes.

Completion check:
- `todo.md` names at least one multi-row RV64 consumer sub-family or records a
  clear no-breadth blocker.
- Non-RV64 owner rows retain accurate diagnostics and are not pulled into this
  route.

### Step 3: Implement one verified cast consumer sub-family

Goal: Add semantic RV64/MIR consumer lowering for the selected cast family.

Primary target:
- The narrow RV64/MIR consumer surface proven by Steps 1 and 2.

Concrete actions:
- Inspect the existing lowering path for the selected cast family.
- Add or adjust the consumer lowering rule without testcase-shaped matching.
- Preserve diagnostics for rows still missing producer or authority facts.
- Keep unrelated pointer, ABI, local-memory, select, branch, global, runtime,
  move-bundle, and terminator behavior unchanged.

Completion check:
- The selected cast sub-family progresses multiple refreshed rows, or
  `todo.md` documents why implementation is blocked after evidence review.
- Nearby non-cast guard rows remain outside the implementation route.
- Fresh build or compile proof is recorded in `test_after.log`.

### Step 4: Prove breadth and guardrails

Goal: Validate that the route repaired a cast capability rather than one named
case.

Concrete actions:
- Re-run the cast diagnostic subset and compare against the Step 1 evidence.
- Include nearby non-cast guard rows from the refresh set.
- Run the supervisor-selected build/test subset for the touched surface.
- Record any remaining cast rows by owner and next action.

Completion check:
- Proof shows multi-row cast progress or a justified no-breadth blocker.
- No expectation, unsupported-marker, allowlist, timeout, or accounting-only
  change is used as the basis for progress.
- `todo.md` contains proof commands, results, residual owner buckets, and
  follow-up notes for supervisor review.
