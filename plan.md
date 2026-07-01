# RV64 Runtime Mismatch Triage Runbook

Status: Active
Source Idea: ideas/open/423_rv64_runtime_mismatch_triage.md
Activated from: open idea inventory after no active plan/todo state

## Purpose

Classify RV64 gcc_torture rows that already compile, link, and execute but
diverge from clang through aborts, segfaults, exit-code mismatches, output
mismatches, or timeouts.

## Goal

Produce evidence-backed runtime mismatch ownership notes and follow-up
implementation ideas without patching runtime behavior in this triage route.

## Core Rule

Do not fix or mask runtime mismatches in this plan. Find the first wrong edge,
assign ownership, and keep unclear rows fail-closed.

## Read First

- `ideas/open/423_rv64_runtime_mismatch_triage.md`
- `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`
- `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`, if present
- `build/agent_state/rv64_gcc_c_torture_backend_failed.full.txt`, if present
- matching per-case logs under `build/rv64_gcc_c_torture_backend/`

## Current Targets And Scope

- Runtime rows where c4c emits and links an RV64 executable, then diverges from
  clang at run time.
- Buckets: abort, segfault, wrong exit code, wrong output, and timeout.
- Evidence: case logs, object/disassembly inspection, focused prepared dumps,
  and any preserved full-scan summaries.
- Outputs: triage notes under the RV64 gcc_torture handoff area and follow-up
  implementation ideas when recurring owners are clear.

## Non-Goals

- Do not implement runtime fixes in this plan.
- Do not weaken runtime comparison, expected output, allowlists, unsupported
  markers, or pass/fail accounting.
- Do not treat unsupported compile failures as runtime mismatches.
- Do not route through F128 unless row evidence proves F128 owns the runtime
  mismatch family.
- Do not infer facts in RV64 from filenames, function names, testcase names,
  block indexes, or raw BIR/source shapes.

## Working Model

- A runtime mismatch is higher risk than fail-closed unsupported codegen
  because object emission has already produced executable code.
- Each representative row needs a first wrong value, address, branch, call, or
  storage-home edge before it can justify an implementation idea.
- If the first wrong edge needs missing prepared/BIR authority, stop at a
  producer-owned follow-up idea rather than consuming inferred facts in RV64.
- If ownership is unclear, preserve the row as triage evidence and avoid a
  speculative patch route.

## Execution Rules

- Keep this runbook as triage and idea generation; implementation belongs to
  follow-up ideas.
- Prefer row-level evidence over aggregate counts when assigning ownership.
- Keep representative artifacts in a dedicated handoff subdirectory when new
  evidence is generated.
- Split broad families into separate follow-up ideas by owning layer.
- Include concrete reviewer reject signals in any new follow-up idea.
- For any code-changing follow-up proposed later, require build proof, focused
  representative proof, and default CTest stability; this runbook itself is
  lifecycle/triage work unless it creates scripts or code.

## Step 1: Inventory Runtime Mismatch Rows

Goal: identify the current rows that compile, link, execute, and diverge.

Primary target: current RV64 gcc_torture backend summary and per-case logs.

Actions:

- Inspect the freshest full-scan summary artifacts and their provenance.
- Extract only rows whose failure mode is run-time divergence, not fail-closed
  compile/codegen unsupported diagnostics.
- Partition rows into abort, segfault, wrong exit code, wrong output, and
  timeout buckets when the logs support that split.
- Preserve the command or artifact path used to derive the row set.

Completion check:

- A durable row inventory exists with counts, representative case IDs, failure
  modes, and source log paths.

## Step 2: Select Representatives And Reproduce

Goal: choose representative rows for each runtime mismatch family and confirm
they still reproduce locally.

Primary target: per-case logs and focused c4c/clang executable pairs.

Actions:

- Pick representatives across distinct failure modes and suspected owners.
- Re-run or reuse focused case artifacts to capture c4c and clang behavior.
- Record return codes, signal names, stdout/stderr differences, and runner
  commands for each representative.
- Drop rows from the runtime bucket if reproduction proves they are actually
  unsupported compile/codegen failures.

Completion check:

- Each selected representative has reproducible behavior and enough command
  detail for another agent to repeat it.

## Step 3: Find First Wrong Edge

Goal: identify the earliest observable wrong value, address, branch, call, or
storage-home edge for each representative family.

Primary target: disassembly, prepared/object dumps, and focused execution
evidence.

Actions:

- Compare c4c and clang control flow and data movement around the divergence.
- Use prepared/object evidence to decide whether the required facts are
  available to RV64 lowering.
- Classify each representative as RV64 object lowering, MIR lowering, BIR or
  prepared producer, runtime/helper, test infrastructure, or unclear.
- Stop before inventing producer facts in the target layer.

Completion check:

- Representative rows have ownership notes tied to first-wrong-edge evidence,
  not only final outputs.

## Step 4: Group Recurring Owners

Goal: convert representative evidence into reusable runtime mismatch families.

Primary target: grouped triage notes in the handoff directory.

Actions:

- Merge representatives that share the same first wrong edge and owning layer.
- Keep singletons separate when the evidence does not justify a broad family.
- Rank groups by correctness risk, row count, ordinary C relevance, and
  implementation readiness.
- Mark F128-owned groups as low-priority quarantine unless they block broader
  non-F128 work.

Completion check:

- Runtime mismatch groups have counts, representatives, owning-layer
  classifications, and route notes.

## Step 5: Create Follow-Up Ideas Or Park Unclear Rows

Goal: publish implementation-ready follow-up ideas only for high-confidence
families and preserve unresolved triage evidence.

Primary target: `ideas/open/` and the RV64 gcc_torture handoff directory.

Actions:

- Create follow-up ideas for recurring high-confidence runtime bug families.
- Put producer-owned gaps in producer ideas before any dependent RV64 consumer
  route.
- Include reviewer reject signals that block testcase-shaped runtime fixes,
  expectation changes, and source-name or row-name shortcuts.
- Leave unclear rows as fail-closed triage evidence with the missing proof
  named explicitly.

Completion check:

- At least one high-confidence runtime implementation idea exists if the bucket
  has a recurring owner; otherwise the handoff explains why no implementation
  idea is justified yet.

## Step 6: Lifecycle Close Check

Goal: decide whether the source idea is complete after triage outputs exist.

Primary target: source idea acceptance criteria and default CTest.

Actions:

- Verify runtime mismatch rows have concrete ownership notes and representative
  artifacts.
- Verify follow-up ideas, if any, are split by owning layer and do not bundle
  implementation with triage.
- Run or request the close-time regression guard required by the lifecycle
  workflow before closure.

Completion check:

- The plan owner can decide whether to close the source idea or replace the
  runbook with a narrower follow-up triage route.
