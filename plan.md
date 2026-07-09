# Runtime Mismatch Ownership Investigation Runbook

Status: Active
Source Idea: ideas/open/618_runtime_mismatch_ownership_investigation.md

## Purpose

Turn current RV64 runtime failures into concrete ownership documentation before
any implementation work is attempted.

## Goal

Produce `docs/runtime_mismatch_ownership/` with one `index.md` and exactly
three numbered answer files that map runtime symptoms to likely first owners
and follow-up implementation splits.

## Core Rule

This is a research/documentation plan only. Do not change implementation code,
tests, expected outputs, unsupported markers, allowlists, timeout policy,
runtime comparison behavior, or lifecycle files outside this activation state.

## Read First

- `ideas/open/618_runtime_mismatch_ownership_investigation.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
- Current RV64 gcc torture backend logs under `build/rv64_gcc_c_torture_backend/`
- Current scan summaries under `build/agent_state/`, when present

## Required Output

Create exactly these documentation files:

- `docs/runtime_mismatch_ownership/index.md`
- `docs/runtime_mismatch_ownership/01_runtime_symptom_map.md`
- `docs/runtime_mismatch_ownership/02_likely_first_owner_map.md`
- `docs/runtime_mismatch_ownership/03_followup_implementation_queue.md`

## Non-Goals

- Do not implement runtime, ABI, layout, memory, call-lowering, or codegen fixes.
- Do not weaken runtime comparison, timeout policy, pass/fail accounting, or
  allowlist coverage.
- Do not mark runtime rows unsupported or rewrite test expectations.
- Do not collapse all runtime symptoms into one generic runtime-support owner.
- Do not edit `ideas/open/618_runtime_mismatch_ownership_investigation.md`
  unless source intent itself changes.

## Working Model

Runtime mismatches are post-object-emission symptoms, not owners by themselves.
Each row must be classified by observed symptom first, then mapped to the
likely first owner using concrete evidence from logs, emitted behavior, current
scan summaries, and known prerequisite blocker families.

Allowed owner buckets are:

- ABI
- layout
- local/global memory
- call lowering
- true runtime support
- unresolved

## Execution Rules

- Keep all research output under `docs/runtime_mismatch_ownership/`.
- Cite concrete logs, commands, qemu results, signals, abort sites, wrong output,
  or timeout evidence for each claim.
- Separate rows that need rerun after prerequisite ideas from rows with enough
  evidence for a current owner classification.
- Recommended implementation work must be split into single-owner follow-up
  ideas or clearly labeled as discussion/policy work.
- Validation is documentation-focused: verify required files exist, answer the
  required questions, and confirm no implementation/test/runtime policy files
  changed.

## Step 1: Refresh Runtime Symptom Evidence

Goal: identify the current runtime-failure row set and the artifact proving each
symptom.

Actions:

- Inspect current scan summaries and RV64 gcc torture backend logs.
- Confirm rows that abort, segfault, produce wrong output, or time out.
- Record representative commands and log paths needed by the documentation.
- Separate compile-time blockers and non-runtime owners from the runtime row set.

Completion check:

- The executor can name the current runtime rows by symptom and cite the
  artifacts that prove each classification.
- No documentation output is required yet unless useful as scratch material.

## Step 2: Write Runtime Symptom Map

Goal: create `01_runtime_symptom_map.md`.

Actions:

- Create `docs/runtime_mismatch_ownership/01_runtime_symptom_map.md`.
- Group rows by abort, segfault, wrong output, and timeout.
- Include representative logs, commands, and concrete symptom evidence.
- Conclude which symptom families appear stable enough for owner mapping.

Completion check:

- `01_runtime_symptom_map.md` directly answers its assigned question and does
  not attempt implementation recommendations beyond symptom stability.

## Step 3: Write Likely First Owner Map

Goal: create `02_likely_first_owner_map.md`.

Actions:

- Map each runtime family to ABI, layout, local/global memory, call lowering,
  true runtime support, or unresolved.
- Include evidence for and against each owner choice.
- Identify rows that should be rerun after prerequisite compile-time or
  codegen ideas.
- Keep mixed-owner and unresolved cases explicit.

Completion check:

- `02_likely_first_owner_map.md` provides concrete first-owner reasoning for
  each runtime family and does not hide uncertainty.

## Step 4: Write Follow-Up Queue And Index

Goal: create `03_followup_implementation_queue.md` and `index.md`, then classify
close readiness.

Actions:

- Create `docs/runtime_mismatch_ownership/03_followup_implementation_queue.md`.
- Split any recommended implementation work by owner and proof surface.
- Mark rows that remain discussion or policy issues.
- Create `docs/runtime_mismatch_ownership/index.md` linking all three answer
  files and summarizing the overall result.
- Verify the directory contains exactly the required answer files plus
  `index.md`.

Completion check:

- All four required documentation files exist and answer the source idea.
- No implementation, expectation, unsupported-marker, allowlist, timeout,
  runtime behavior, accounting, or unrelated lifecycle files changed.
- The plan owner can evaluate whether idea `618` is close-ready.
