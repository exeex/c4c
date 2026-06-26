# RV64 Object Route Terminator Fragment Lowering Runbook

Status: Active
Source Idea: ideas/open/369_rv64_object_route_terminator_fragment_lowering.md

## Purpose

Repair the next RV64 object-route prepared terminator-fragment blocker exposed
after data, string, relocation, and multi-block text support.

## Goal

Identify the first unsupported prepared terminator-fragment shape behind the
representative failures, admit one semantic RV64 object lowering slice when it
is supportable, and prove the representatives either pass or advance to
distinct next owners.

## Core Rule

Use prepared CFG, terminator, edge, publication, and block metadata as the
source of truth. Do not infer branch destinations, fallthrough behavior, or
condition semantics from testcase names, source syntax, instruction indexes,
block numbering accidents, or prepared-BIR text matching.

## Read First

- Source idea:
  `ideas/open/369_rv64_object_route_terminator_fragment_lowering.md`
- Parent classification:
  `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
- Likely evidence:
  - `build/agent_state/354_step3_representative_refresh.log`
  - `build/rv64_gcc_c_torture_backend/src_20000224-1.c/case.log`
  - `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`
- Likely implementation surface:
  - `src/backend/mir/riscv/codegen/object_emission.cpp`
  - `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Scope

- Representatives:
  - `src/20000224-1.c`
  - `src/20000112-1.c`
- Current diagnostic:
  `unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering`
- Focused backend coverage should include:
  - `backend_prepare_frame_stack_call_contract`
  - `backend_prepared_printer`
  - `backend_riscv_object_emission`

## Non-Goals

- Do not reopen data-section, global-symbol, string-constant, relocation, or
  multi-block text support from prior ideas.
- Do not reconstruct CFG, branch targets, edge order, or value publication by
  scanning C source or raw prepared text.
- Do not implement frame-slot base-plus-offset local memory, byval aggregate
  parameter homes, variadic helper lowering, non-register parameter homes, or
  unrelated call-argument ownership.
- Do not downgrade unsupported tests, weaken contracts, filter allowlists, or
  claim diagnostic-only work as backend capability progress.
- Do not add testcase-shaped or named-case shortcuts for the representatives.

## Working Model

The representatives reached RV64 object emission and now stop at the prepared
terminator-fragment gate. The first executor packet should establish the exact
prepared terminator shape in CFG terms before any lowering is admitted.
Support should be added only for terminators whose condition, successor, label,
and publication facts are explicit in the prepared module contract. Adjacent
terminator classes must remain fail-closed with precise diagnostics.

## Execution Rules

- Keep each code slice focused on one admitted terminator shape or one
  diagnostic refinement needed to identify that shape.
- Add focused object-emission tests for the admitted shape and nearby rejected
  shapes before treating the implementation as complete.
- Preserve existing prepared-contract tests unless the source idea explicitly
  requires a contract change.
- Update `todo.md` after each executor packet with the current step, evidence,
  proof command, result, and any next owner.
- If a representative advances to a distinct out-of-scope owner, stop and ask
  for lifecycle closure or handoff instead of expanding this plan.

## Step 1: Audit First Terminator Fragment

Goal: identify the earliest prepared terminator-fragment shape rejected by the
RV64 object route for the listed representatives.

Primary targets:

- `src/20000224-1.c`
- `src/20000112-1.c`

Actions:

- Regenerate or inspect prepared/object-route evidence for both
  representatives.
- Locate the first `unsupported_terminator_fragment` owner in prepared CFG and
  terminator terms.
- Document the terminator opcode/class, condition value and home if present,
  successor labels, fallthrough behavior, block layout assumptions, and any
  required publication behavior.
- If the generic diagnostic blocks precise ownership, make only the minimum
  diagnostic improvement needed for the audit.
- Decide whether both representatives share the same first supportable
  terminator shape or require separate packets.

Completion check:

- `todo.md` names one concrete prepared terminator shape for implementation, or
  names the out-of-scope owner if no in-scope shape is present.
- Evidence artifacts are recorded under `build/agent_state/` or the per-case
  log paths.

## Step 2: Add Focused Terminator Tests

Goal: lock the admitted prepared terminator shape and adjacent fail-closed
contracts before or alongside implementation.

Primary target:

- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Actions:

- Add a positive object-emission test for the selected prepared terminator
  shape.
- Add nearby negative tests for unsupported terminator classes, missing CFG
  facts, unsupported condition homes or types, ambiguous successors, or
  unsupported publication behavior.
- Keep tests based on prepared terminator metadata, not the gcc-torture
  testcase names.

Completion check:

- The focused tests fail for the missing capability or pass only after a
  semantic implementation, and rejected adjacent shapes retain precise
  diagnostics.

## Step 3: Implement Semantic RV64 Terminator Emission

Goal: emit RV64 object code for the selected prepared terminator-fragment
shape.

Primary target:

- `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:

- Extend the terminator-fragment emission path for the selected supported
  prepared terminator shape.
- Use existing RV64 emission helpers, prepared labels, block layout, condition
  homes, and publication data where possible.
- Keep unsupported terminators, edge layouts, condition homes, or ambiguous CFG
  facts on explicit fail-closed paths.
- Avoid broad rewrites outside the object-route terminator gate.

Completion check:

- Focused object-emission tests prove the admitted terminator shape and
  adjacent rejected shapes.

## Step 4: Validate Focused Backend Coverage

Goal: prove the terminator change does not break nearby prepared/object
emission contracts.

Proof command:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test &&
ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Completion check:

- The command succeeds and `test_after.log` is the canonical proof log for the
  executor packet.

## Step 5: Rerun Representatives

Goal: check whether the representative cases now pass the RV64 object route or
advance to distinct next owners.

Proof command:

```sh
ALLOWLIST=build/agent_state/369_step5_terminator_representatives.allowlist.txt STOP_ON_FAILURE=1 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/369_step5_terminator_representatives.runner.log 2>&1
```

Actions:

- Create the allowlist with only:
  - `src/20000224-1.c`
  - `src/20000112-1.c`
- Inspect both the runner log and per-case logs after the run.
- Confirm the prior terminator-fragment shape no longer fails.
- If a new failure appears, classify whether it remains in-scope for this idea
  or is a distinct next owner.

Completion check:

- `todo.md` records each representative result and supporting log paths.
- If a result is out-of-scope, `todo.md` includes enough detail for lifecycle
  closure and follow-up ownership.

## Step 6: Closure Or Handoff Check

Goal: decide whether the source idea is complete under its acceptance criteria
or whether another in-scope terminator packet is required.

Actions:

- Verify the unsupported terminator-fragment shape was documented.
- Verify admitted lowering has focused backend tests and fail-closed adjacent
  coverage.
- Verify existing focused backend object-emission and prepared-contract
  coverage remains green.
- Verify both representatives either pass or advance to documented distinct
  next owners.

Completion check:

- `todo.md` clearly states either the next in-scope executor packet or that
  source-idea closure/handoff is ready.
