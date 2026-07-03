# RV64 Integer Div/Rem Instruction-Fragment Lowering Runbook

Status: Active
Source Idea: ideas/open/567_rv64_integer_div_rem_instruction_fragment_lowering.md

## Purpose

Close the RV64 object-lowering gap for coherent BIR integer division and
remainder instructions from the current `unsupported_instruction_fragment`
classification.

## Goal

Generalize RV64/MIR object emission for BIR `sdiv`, `udiv`, `srem`, and
`urem` so the routed `30` `integer_div_rem` rows no longer fail for the
div/rem-owned instruction-fragment diagnostic.

## Core Rule

Implement semantic BIR div/rem lowering. Do not use testcase names, raw
diagnostic text, allowlist behavior, or expectation changes as evidence of
capability progress.

## Read First

- `ideas/open/567_rv64_integer_div_rem_instruction_fragment_lowering.md`
- `ideas/closed/546_rv64_instruction_fragment_current_classification.md`
- `build/agent_state/546_step3_instruction_fragment_classification.tsv`
- `build/agent_state/546_step4_instruction_fragment_screening.tsv`
- `build/agent_state/546_step5_followup_routing.md`
- `build/agent_state/546_step5_followup_routing.tsv`
- `build/agent_state/unsupported_instruction_fragment_current_rows.tsv`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/alu.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `scripts/check_progress_rv64_gcc_c_torture_backend.sh`

## Current Targets

- Operation family: BIR `sdiv`, `udiv`, `srem`, `urem`
- Owner: `rv64_object_lowering`
- Routed row count: `30`
- Representative rows:
  - `src/20001026-1.c`
  - `src/20050215-1.c`
  - `src/20090113-2.c`
  - `src/20090113-3.c`
  - `src/20101013-1.c`
- Expected implementation surfaces:
  - RV64 object traversal that currently reports
    `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`
  - RV64 scalar ALU instruction selection for `div`, `divu`, `rem`, `remu`
    and their 32-bit `*w` forms
  - Backend object-emission tests that prove signed/unsigned division and
    signed/unsigned remainder

## Non-Goals

- Do not lower arithmetic shift-right, pointer/integer casts, F32/F64 scalar
  FP operations, or heterogeneous scalar integer buckets.
- Do not touch F128 or long-double policy.
- Do not infer producer/prepared, ABI/call, or evidence-gap ownership.
- Do not change unsupported markers, pass/fail accounting, allowlists, or
  expectations as a substitute for lowering support.
- Do not add helper-call substitutions unless the active route is explicitly
  changed to runtime-helper lowering.

## Working Model

The source idea owns only rows where current BIR evidence already contains
coherent integer div/rem operations and Step 4 classified the first owner as
RV64 object lowering. The executor should first confirm the exact current
lowering boundary, then add one generalized path that consumes semantic BIR
opcodes and emits the correct RV64 M-extension instruction for each operation
and width.

## Execution Rules

- Keep row evidence tied to the refreshed 2026-07-03 coherent scan artifacts.
- Preserve signed versus unsigned semantics for every opcode.
- Preserve 32-bit versus 64-bit operand-width behavior; use `divw`, `divuw`,
  `remw`, and `remuw` where the existing BIR/prepared facts require I32/U32.
- Add focused backend coverage before claiming representative progress.
- Prove at least one representative allowlist drawn from the routed div/rem
  rows after the focused backend proof is green.
- If a representative row advances to a different diagnostic, record the new
  downstream owner in `todo.md`; do not claim that owner inside this idea.
- If evidence shows the row is not actually div/rem-owned, route that fact in
  `todo.md` instead of stretching this plan.

## Step 1: Reconstruct Div/Rem Lowering Boundary

Goal: Confirm the current semantic boundary for the routed div/rem rows and
the exact object-emission site that must change.

Primary targets:

- `build/agent_state/546_step4_instruction_fragment_screening.tsv`
- `build/agent_state/546_step5_followup_routing.tsv`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/alu.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- Representative allowlist log under `build/agent_state/`

Actions:

- Extract the `30` `integer_div_rem` rows from the Step 4/Step 5 artifacts and
  record the row list or artifact path in `todo.md`.
- Run a small representative allowlist before code changes, including at least
  `src/20001026-1.c`, `src/20050215-1.c`, `src/20090113-2.c`,
  `src/20090113-3.c`, and `src/20101013-1.c`, unless the supervisor delegates a
  different subset.
- Inspect the RV64 object traversal and scalar ALU helpers to identify the
  semantic hook for BIR binary opcodes.
- Record whether existing code already selects RV64 div/rem opcodes and what
  facts are missing from object emission.
- Update `todo.md` with the baseline diagnostic, representative command, and
  proposed narrow code target.

Completion check:

- `todo.md` names the authoritative 30-row input, baseline representative
  result, and the concrete RV64 object-emission hook for Step 2.

## Step 2: Add Generalized RV64 Div/Rem Object Lowering

Goal: Lower BIR `sdiv`, `udiv`, `srem`, and `urem` through one generalized
RV64 object-emission path.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/alu.cpp`
- Existing RV64 object-emission operand/register helpers

Actions:

- Consume semantic BIR binary opcodes, not testcase names or diagnostic text.
- Map signed division to `div`/`divw`.
- Map unsigned division to `divu`/`divuw`.
- Map signed remainder to `rem`/`remw`.
- Map unsigned remainder to `remu`/`remuw`.
- Reuse existing operand materialization and result publication patterns where
  possible.
- Fail closed with a concrete diagnostic if required operand/result facts are
  missing or inconsistent.
- Keep non-div/rem instruction-fragment cases on their existing route.

Completion check:

- Focused backend tests for all four opcode families pass locally.
- Existing non-div/rem unsupported-instruction tests still preserve their
  expected diagnostic behavior.

## Step 3: Add Focused Object-Emission Coverage

Goal: Prove the generalized lowering contract independently of named
gcc_torture cases.

Primary target:

- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Actions:

- Add or extend focused RV64 object-emission tests for:
  - signed division
  - unsigned division
  - signed remainder
  - unsigned remainder
  - 32-bit and 64-bit width selection where the local helpers support both
- Include at least one fail-closed malformed-fact case if Step 2 adds new
  validation branches.
- Avoid weakening any existing unsupported-instruction assertions.

Completion check:

- The focused tests fail before the semantic code change or are otherwise
  shown to exercise the changed branch.
- `cmake --build --preset default` and the supervisor-selected backend subset
  pass.

## Step 4: Prove Representative Div/Rem Rows

Goal: Show that the representative routed rows no longer fail for the
div/rem-owned `unsupported_instruction_fragment` gap.

Primary targets:

- `scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- Representative allowlist artifact under `build/agent_state/`
- Current `test_after.log` for backend subset proof

Actions:

- Run the supervisor-delegated backend proof command.
- Run a representative allowlist including at least the source-idea examples:
  `src/20001026-1.c`, `src/20050215-1.c`, `src/20090113-2.c`,
  `src/20090113-3.c`, and `src/20101013-1.c`.
- Confirm the original div/rem-owned unsupported diagnostic is gone for those
  rows.
- Record any new residual diagnostic and downstream owner in `todo.md`.
- If nearby div/rem rows still retain the same owned diagnostic, do not accept
  the slice until Step 2 is generalized or Step 1 ownership is corrected.

Completion check:

- `todo.md` records backend proof, representative allowlist proof, old
  diagnostic removal, and any concrete downstream residual owner.

## Step 5: Closure Check And Residual Routing

Goal: Decide whether the source idea is complete or needs a narrowed repair
packet.

Actions:

- Verify focused tests cover all four div/rem operation families.
- Verify representative rows no longer fail for the owned div/rem lowering gap.
- Compare remaining residuals against existing open ideas before creating new
  source ideas.
- If remaining same-family rows still fail with the old gap, keep the plan
  active and write the next packet in `todo.md`.
- If the source criteria are satisfied, request plan-owner closure through the
  supervisor.

Completion check:

- The source idea can be closed only when generalized div/rem lowering is
  proven and residuals, if any, are assigned to concrete downstream owners.
