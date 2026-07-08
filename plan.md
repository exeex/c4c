# BIR Local-Memory Load Semantics Runbook

Status: Active
Source Idea: ideas/open/602_bir_local_memory_load_semantics.md

## Purpose

Repair BIR production of local-memory load semantics for ordinary C cases so
prepared and RV64 stages receive valid memory-use facts instead of stopping at
the semantic producer layer.

## Goal

Move multiple RV64 gcc_torture backend-object rows from the local-memory load
diagnostic family past the original BIR producer stop while preserving accurate
first-owner diagnostics for adjacent memory families.

## Core Rule

Fix semantic BIR load production. Do not paper over missing load facts in RV64
target code, expectations, unsupported markers, allowlists, timeout handling,
runtime comparison, or pass/fail accounting.

## Read First

- ideas/open/602_bir_local_memory_load_semantics.md
- docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md
- docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md
- docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md

## Current Targets

- First owner: BIR semantic producer
- Primary family: local-memory load rows in the RV64 gcc_torture backend-object route
- Expected breadth: the `82` local-memory load rows identified by the failure map
- Proof surface: representative same-family rows plus nearby local-memory rows
  whose true first owner is store, GEP, alloca, prepared authority, or RV64
  consumption

## Non-Goals

- Store, GEP, alloca, global initializer, RV64 consumer, ABI, or runtime fixes.
- Expectation, unsupported-marker, allowlist, timeout, runtime comparison, or
  pass/fail accounting changes.
- Treating final assembly shape as proof that BIR load semantics are correct.
- A broad local-memory rewrite that changes adjacent owner families without
  separate lifecycle ownership.

## Working Model

The current scan stops a large group of ordinary C local-memory load cases at a
BIR producer diagnostic. This plan should identify the missing BIR fact, repair
the semantic producer in the narrow load path, and then prove the repair across
more than one case in the same family. Rows whose real blocker is store, GEP,
alloca, prepared authority, or RV64 consumption must remain honestly
classified.

## Execution Rules

- Use semantic lowering or fact production, not testcase-shaped matching.
- Keep named cases such as `src/20041124-1.c` as probes only, never as match
  keys.
- Preserve diagnostics that distinguish load producer failures from adjacent
  local-memory families.
- Keep each code-changing step build-backed. The supervisor owns the exact
  proving subset, but executor packets should record the commands and results
  in `todo.md` and `test_after.log`.
- Escalate to plan review instead of expanding into store, GEP, alloca, ABI,
  runtime, or RV64 consumer ownership.

## Ordered Steps

### Step 1. Select Representative Load-Family Proof Rows

Goal: Build a small, documented proof set from the current failure evidence.

Actions:
- Inspect the current scan summary and failure bucket map for rows classified
  in the local-memory load semantic family.
- Select multiple representative load-family rows, including at least one
  simple row and one row near adjacent local-memory behavior if available.
- Select a small guard set of adjacent rows whose first owner should remain
  store, GEP, alloca, prepared authority, or RV64 consumption.
- Record the selected rows and the intended proof command in `todo.md`.

Completion check:
- `todo.md` names the load-family proof rows, adjacent guard rows, and the
  supervisor-provided or proposed RV64 backend-object proof command.

### Step 2. Locate the Missing BIR Load Fact

Goal: Identify the BIR production path that loses or rejects local-memory load
semantics.

Actions:
- Trace the selected rows through the frontend, HIR/BIR production, and the
  prepared/RV64 handoff until the original producer stop is reached.
- Inspect the BIR load, local frame memory, scalar value, and diagnostic paths
  that decide whether a load fact is usable.
- Confirm that the missing behavior belongs to load production and not store,
  GEP, alloca, prepared authority, or RV64 consumption.
- Record the owning functions/files and the old failure signature in `todo.md`.

Completion check:
- The packet identifies a BIR producer root cause and a narrow implementation
  target, with adjacent families explicitly ruled in or out.

### Step 3. Repair Local-Memory Load Production

Goal: Produce valid BIR local-memory load semantics for the selected ordinary C
cases.

Actions:
- Implement the narrow BIR semantic producer change for local frame memory and
  scalar-value loads.
- Preserve existing diagnostics for nearby rows that still lack store, GEP,
  alloca, prepared authority, or RV64 consumer support.
- Avoid changes in RV64 target lowering, expectations, unsupported markers,
  allowlists, timeout handling, runtime comparison, or accounting.
- Run a build or compile proof before testcase proof.

Completion check:
- The implementation builds, and the selected load-family rows progress past
  the original BIR producer stop without weakening adjacent diagnostics.

### Step 4. Prove Same-Family Breadth

Goal: Show the repair generalizes beyond a single named testcase.

Actions:
- Run the delegated RV64 gcc_torture backend-object subset for the selected
  load-family rows.
- Add nearby same-family rows if the first proof is too narrow to demonstrate
  semantic breadth.
- Run the adjacent guard rows and verify their first-owner diagnostics remain
  accurate.
- Save proof output in `test_after.log` unless the supervisor delegates another
  artifact.

Completion check:
- Multiple load-family rows progress past the old stop, and guard rows do not
  get forced through load repair.

### Step 5. Update Evidence and Handoff

Goal: Leave clear evidence for the next lifecycle route.

Actions:
- Update `todo.md` with the final row set, commands, results, and any remaining
  local-memory load limitations.
- If the repair exposes the next owner for rows in the broader queue, document
  that handoff in `todo.md` without editing other open ideas.
- Request supervisor review if the route needs to expand beyond BIR load
  semantics.

Completion check:
- `todo.md` contains the final proof summary, residual risks, and next suggested
  route; no unrelated lifecycle or expectation files were changed.

## Acceptance Check

This runbook is complete when multiple local-memory load rows from the current
RV64 gcc_torture evidence progress past the original BIR producer stop, nearby
non-load owner families remain accurately classified, and proof is recorded
without expectation, unsupported-marker, allowlist, timeout, runtime, accounting,
or RV64 target paper-over changes.

## Reviewer Reject Signals

- Testcase-shaped matching against representative case names such as
  `src/20041124-1.c`.
- Expectation, unsupported-marker, allowlist, timeout, runtime comparison, or
  pass/fail accounting changes claimed as progress.
- RV64 target inference that papers over missing BIR load facts.
- Broad local-memory rewrites that also change store, GEP, alloca, ABI, or
  runtime behavior without separate ownership.
- Helper renames or diagnostic wording changes that leave the original load
  semantic failure mode intact.
