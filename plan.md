# RV64 Select And Phi-Select Lowering Runbook

Status: Active
Source Idea: ideas/open/573_rv64_select_phi_select_lowering.md
Activated from: open RV64 unsupported-instruction diagnostics follow-up

## Purpose

Implement RV64 object-route lowering for BIR scalar integer `SelectInst`
values, including prepared phi-select publication shapes identified by the
570 diagnostics.

## Goal

Lower scalar integer select values through RV64 object emission with focused
proof for simple selects and nested/phi-select publication, while preserving
fail-closed diagnostics for unsupported select forms.

## Core Rule

Do not make `src/20030408-1.c`, `test1`, `logic.end.117`, `%t126.phi.sel0`,
block names, value names, or diagnostic artifact paths part of the repair
contract.

## Read First

- `ideas/open/573_rv64_select_phi_select_lowering.md`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/classification.tsv`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20030408-1.c/dump-prepared-bir.txt`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20030408-1.c/object-route.log`

## Current Targets

- RV64 object emission for scalar integer BIR `SelectInst` values.
- Nested select chains where prepared owners such as `%*.phi.sel*` publish the
  selected result.
- Narrow select-specific diagnostics for operand or type forms that remain
  unsupported.

## Non-Goals

- Do not rebuild general branch lowering, CFG reconstruction, or BIR producer
  semantics unless focused evidence proves the prepared select representation
  is invalid.
- Do not mix in same-module call, inline asm carrier, floating-point binary,
  pointer arithmetic, or runtime comparison work.
- Do not edit expectations, unsupported markers, allowlists, runtime
  comparison contracts, or gcc_torture runner behavior.
- Do not add filename-specific, function-specific, block-specific, or
  emitted-name-specific handling.

## Working Model

The 570 diagnostics classified `src/20030408-1.c` as a distinct select owner
family. Prepared BIR contains nested scalar integer select chains published
from logic joins, and the first object-route unsupported instruction is the
select value materialization itself. This runbook treats that as an RV64
object-emission capability gap until direct evidence proves otherwise.

## Execution Rules

- Start from the saved 570 diagnostics and reproduce the current failure before
  changing lowering behavior.
- Add or identify focused coverage for the semantic select shape before or
  with the repair.
- Keep changes scoped to scalar integer select materialization and result
  publication.
- Preserve unsupported fallback behavior for unhandled select types or operand
  forms, but make diagnostics select-specific when possible.
- If evidence identifies a separate first owner, stop and route that through
  lifecycle state instead of expanding this plan.
- Treat classification-only diffs, helper renames, expectation edits, and
  unsupported-marker changes as non-progress.
- Use the supervisor-delegated proof command for each packet and record proof
  in `todo.md`; broader validation is a supervisor decision.

## Ordered Steps

### Step 1: Rehydrate Select Evidence And Reproduce

Goal: confirm the current object-route failure is still scalar integer select
materialization.

Primary Target: saved 570 select artifacts and the current RV64 object-route
representative rerun.

Actions:

- Inspect the saved classification row, prepared BIR, and object-route log
  listed in Read First.
- Reproduce the current `src/20030408-1.c` RV64 object-route behavior using the
  supervisor-delegated command.
- Record whether the first unsupported owner remains a scalar integer
  `SelectInst` and whether it is a simple select or nested/phi-select
  publication shape.

Completion Check:

- `todo.md` records the artifact paths, rerun command, result, and current
  first unsupported select owner or the exact blocker proving the route moved.

### Step 2: Classify The Lowering Boundary

Goal: identify the precise RV64 lowering/publication path responsible for the
unsupported select.

Primary Target: RV64 object lowering for prepared scalar values and select
result publication.

Actions:

- Trace the prepared select instruction from BIR through RV64 object emission.
- Determine whether the missing behavior is compare materialization, operand
  selection, result carrier publication, nested select sequencing, or a type
  support check.
- Record concrete evidence from prepared BIR, lowering code, diagnostics, MIR,
  object output, or equivalent artifacts.

Completion Check:

- `todo.md` names the first lowering boundary and distinguishes it from branch
  lowering, call lowering, pointer arithmetic, FP binary lowering, and runtime
  comparison work.

### Step 3: Add Focused Select Coverage

Goal: protect the supported scalar integer select behavior without overfitting
the representative.

Primary Target: the narrowest backend or route-level test bucket that can
express scalar integer select materialization.

Actions:

- Add or extend focused coverage for a simple scalar integer select.
- Add or extend focused coverage for a nested or phi-select publication shape
  if the classified route requires it.
- Add fail-closed coverage or assertions for at least one unsupported select
  form when practical.
- Avoid representative filenames, function names, block names, value names, and
  artifact-specific strings.

Completion Check:

- The focused coverage fails before the semantic repair or is justified as
  exercising the classified missing behavior, and it passes after the repair.

### Step 4: Implement Scalar Integer Select Lowering

Goal: emit RV64 object code for the classified scalar integer select shape and
publish the selected result correctly.

Primary Target: the RV64 object-emission path identified in Step 2.

Actions:

- Implement the smallest semantic repair for scalar integer `SelectInst`
  lowering.
- Materialize the condition and selected operands according to existing RV64
  register/value publication conventions.
- Preserve nested/phi-select result publication for prepared owners such as
  `%*.phi.sel*`.
- Keep unsupported select types fail-closed with a narrower diagnostic.
- Re-run the focused proof and the representative command delegated by the
  supervisor.

Completion Check:

- Focused select coverage passes, the representative no longer reaches the old
  generic `unsupported_instruction_fragment` first failure for scalar integer
  select materialization, and `todo.md` records exact proof commands and
  results.

### Step 5: Guard Adjacent RV64 Object Routes

Goal: prove the select repair did not regress adjacent RV64 object-emission
families or recently repaired select publication behavior.

Primary Target: supervisor-selected backend buckets and representatives that
cover prepared-value publication, logical/select publication, and unsupported
instruction diagnostics.

Actions:

- Run the supervisor-selected guard subset for prepared-value/select
  publication behavior.
- Run the supervisor-selected guard subset for the 570 unsupported-instruction
  diagnostic family when required.
- Escalate to broader validation if the repair touches shared branch,
  compare, register-publication, or object-emission machinery.

Completion Check:

- `todo.md` records green guard proof for the selected adjacent subsets, or
  records the exact blocker preventing acceptance.

## Completion Criteria

- Scalar integer `SelectInst` values in the observed prepared-BIR shape lower
  through RV64 object emission or fail with a narrower select-specific
  diagnostic.
- Focused tests cover simple scalar select behavior and nested/phi-select
  publication.
- Unsupported select forms remain fail-closed.
- The representative proof does not depend on `src/20030408-1.c`, function,
  block, or value-name matching.
