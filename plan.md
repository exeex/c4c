# RV64 Instruction-Fragment Current Classification Runbook

Status: Active
Source Idea: ideas/open/546_rv64_instruction_fragment_current_classification.md

## Purpose

Reclassify the current `unsupported_instruction_fragment` RV64 gcc_torture
rows before any implementation slice uses stale historical sub-buckets.

## Goal

Produce a current, row-level classification of the 137
`unsupported_instruction_fragment` failures into RV64 implementation buckets,
producer gaps, and F128 quarantine rows.

## Core Rule

Use the stable 2026-07-02 row evidence as authority. Do not reuse old 190-row
or 82-row instruction-fragment counts as current implementation scope.

## Read First

- `ideas/open/546_rv64_instruction_fragment_current_classification.md`
- `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`
- `scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt`
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/rv64_gcc_c_torture_backend/<case-id>/case.log`

## Current Targets

- Current diagnostic: `unsupported_instruction_fragment`
- Current expected count: 137 rows from the stable 2026-07-02 scan evidence
- Current scan anchor:
  `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`
- Historical taxonomy inputs are allowed as hints only after the current row
  set is reconstructed.

## Non-Goals

- Do not implement instruction lowering in this classification plan.
- Do not edit expectations, unsupported markers, allowlists, or pass/fail
  accounting.
- Do not route missing BIR or prepared facts to RV64 inference work.
- Do not mix primary-F128 or long-double helper rows into ordinary scalar
  instruction-fragment work.
- Do not classify rows by testcase name, raw opcode text alone, or stale count
  tables.

## Working Model

`unsupported_instruction_fragment` means the object route reached a BIR
instruction that the RV64 object lowering layer cannot currently lower. Some
rows may be true RV64 lowering gaps with coherent semantic facts. Other rows
may expose missing BIR/prepared facts or F128 policy cases that should be
split before implementation. Classification must find the first owner before
new implementation ideas are created.

## Execution Rules

- Keep this plan evidence-first. Code changes require a separate follow-up
  idea unless the supervisor explicitly activates an implementation plan.
- Derive the current row table from the mutable 2026-07-02-aligned summary and
  per-case logs, or refresh the full scan if the artifacts are missing.
- Preserve commands, generated artifact paths, row counts, and representative
  samples in `todo.md` as each packet completes.
- Use historical instruction-fragment classifications only to propose labels,
  never as current row counts.
- Screen F128-primary rows before ranking ordinary-C RV64 buckets.
- If a coherent implementation bucket emerges, create or request a narrow
  source idea with concrete row evidence and explicit reviewer reject signals.

## Step 1: Reconstruct Current Instruction-Fragment Rows

Goal: Build a current row-level table for the 137
`unsupported_instruction_fragment` failures.

Primary targets:

- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/rv64_gcc_c_torture_backend/<case-id>/case.log`
- `build/agent_state/unsupported_instruction_fragment_current_rows.tsv`

Actions:

- Verify the current scan pointer still names the 2026-07-02 stable scan.
- Extract failing rows whose per-case logs contain
  `unsupported_instruction_fragment`.
- Write a current TSV artifact with at least case path and per-case log path.
- Confirm the row count is 137, or record the exact drift and evidence source
  if the current artifacts differ.
- Update `todo.md` with the command, output artifact, row count, and any stale
  artifact that must not be reused.

Completion check:

- `todo.md` records a current row artifact, a count check, and the next
  classification packet.

## Step 2: Classify Semantic Families And First Owners

Goal: Assign each current row to a semantic family and first owner.

Primary targets:

- Current row TSV from Step 1
- Per-case logs and adjacent prepared/BIR dumps generated only as needed
- `docs/rv64_gcc_torture_post_contract/` classification notes if a durable
  artifact is needed

Actions:

- Inspect representative rows from each recurring BIR instruction family.
- Classify rows by semantic operation family, prepared fact completeness, and
  likely first owner.
- Separate true RV64 lowering gaps from BIR producer gaps, prepared contract
  gaps, call or aggregate ABI gaps, and evidence gaps.
- Record representative cases for each family.
- Do not create implementation shortcuts from opcode text alone.

Completion check:

- `todo.md` records the classification table location, sub-bucket counts,
  representative rows, and first-owner rationale.

## Step 3: Screen F128 And Producer-Gap Rows

Goal: Remove non-ordinary-C or producer-owned rows from the RV64 implementation
queue before ranking buckets.

Actions:

- Identify primary-F128 or long-double rows and route them to the F128
  quarantine lane.
- Identify rows that require missing BIR semantic facts or prepared authority
  facts before RV64 lowering can consume them.
- Record which rows remain implementation-ready for RV64/MIR object lowering.
- If a producer-owned row family is significant and not covered by an existing
  open idea, recommend a new source idea instead of folding it into RV64
  lowering.

Completion check:

- `todo.md` records screened counts and names the remaining
  implementation-ready ordinary-C RV64 buckets.

## Step 4: Produce Follow-Up Routing

Goal: Turn the classification into actionable lifecycle routing without doing
implementation work in this plan.

Actions:

- Rank high-frequency implementation-ready RV64 buckets by expected value and
  proof clarity.
- Recommend one or more narrow follow-up ideas only when row evidence and
  first-owner proof are concrete.
- Ensure each proposed implementation bucket rejects testcase-shaped lowering,
  expectation rewrites, and unsupported downgrades.
- If no coherent implementation bucket is ready, record the blocking evidence
  gaps and recommend producer-boundary follow-up instead.

Completion check:

- The current 137-row set is traceable to scan evidence.
- Each high-frequency sub-bucket has a first owner and readiness decision.
- F128-primary rows are routed to quarantine.
- Any follow-up implementation idea is narrow, evidence-backed, and ready for
  supervisor lifecycle selection.
