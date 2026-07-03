# RV64 Integer Div/Rem Residual Instruction-Fragment Lowering Runbook

Status: Active
Source Idea: ideas/open/567_rv64_integer_div_rem_instruction_fragment_lowering.md

## Purpose

Clear or reroute the RV64 object-lowering residuals for rows classified under
the current `integer_div_rem` `unsupported_instruction_fragment` family.

## Goal

Pin the first still-unsupported semantic instruction fragment in the routed
`30` `integer_div_rem` rows, then repair only the proven RV64 object-lowering
gap or route the row to its concrete downstream owner.

## Core Rule

Do not add duplicate div/rem opcode lowering. Step 1 proved the raw
`sdiv`/`udiv`/`srem`/`urem` RV64 object path already exists; any next code
change must be driven by the exact later unsupported fragment.

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

- Operation family: rows initially classified from BIR `sdiv`, `udiv`, `srem`,
  and `urem` evidence
- Owner: `rv64_object_lowering`
- Routed row count: `30`
- Representative rows:
  - `src/20001026-1.c`
  - `src/20050215-1.c`
  - `src/20090113-2.c`
  - `src/20090113-3.c`
  - `src/20101013-1.c`
- Known boundary facts from Step 1:
  - `object_emission.cpp` already routes `bir::BinaryInst` through
    `fragment_for_prepared_binary(...)`.
  - `prepared_scalar_emit.cpp` already maps `SDiv`, `UDiv`, `SRem`, and
    `URem` to `div/divu/rem/remu` and `divw/divuw/remw/remuw`.
  - `alu.cpp` already has the corresponding mnemonic selection.
  - Focused object-emission coverage already covers all four operations at
    I32 and I64 width.
  - Representative rows still fail with the generic
    `unsupported_instruction_fragment`, so Step 2 must identify the later
    fragment before implementation.

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

The source idea owns only the routed rows while their first real current owner
is still RV64 object lowering. Step 1 showed that the obvious raw div/rem
encoder is not missing. The executor must now instrument or reproduce the
representative path precisely enough to name the first unsupported instruction
after existing div/rem lowering has had a chance to run.

## Execution Rules

- Keep row evidence tied to the refreshed 2026-07-03 coherent scan artifacts.
- Preserve signed versus unsigned semantics and width behavior if a
  div/rem-adjacent repair is eventually required.
- Do not add more focused div/rem opcode tests unless the new test exercises a
  newly pinned missing semantic path.
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

## Step 2: Pin First Downstream Unsupported Fragment

Goal: Identify the first concrete unsupported BIR/prepared instruction in a
representative routed row after the existing div/rem binary hook.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- Representative BIR and prepared-BIR dumps under `build/agent_state/`
- A focused unit-level reproducer if it can isolate the same fragment

Actions:

- Start from one representative such as `src/20001026-1.c`.
- Determine whether the failing fragment is a later `BinaryInst`, `SelectInst`,
  result publication, memory/home materialization, or another semantic shape.
- Use structured dumps, local tracing, or a focused reproducer; do not classify
  from the generic diagnostic text alone.
- Record the pinned instruction kind, operand/result facts, and exact current
  rejecting hook in `todo.md`.
- If the first owner is not an implementation-ready RV64 object-lowering gap,
  route it in `todo.md` instead of stretching this plan.

Completion check:

- `todo.md` names the first unsupported fragment, the rejecting hook, the
  semantic owner, and the next repair or reroute decision.

## Step 3: Repair Pinned RV64 Object-Lowering Gap

Goal: Add a generalized RV64 object-emission repair only for the semantic gap
identified in Step 2.

Primary target:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Actions:

- Consume semantic BIR/prepared facts, not testcase names, diagnostics, or
  allowlist membership.
- Reuse existing operand materialization and result publication patterns where
  possible.
- Add focused coverage for the newly repaired semantic path.
- Include fail-closed coverage if the repair adds new validation branches.
- Preserve existing non-owned unsupported-instruction behavior.

Completion check:

- The focused tests fail before the semantic code change or are otherwise
  shown to exercise the changed branch.
- `cmake --build --preset default` and the supervisor-selected backend subset
  pass.

## Step 4: Prove Representative Routed Rows

Goal: Show that the representative routed rows no longer fail for the pinned
`unsupported_instruction_fragment` gap or are assigned to a concrete
downstream owner.

Primary targets:

- `scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- Representative allowlist artifact under `build/agent_state/`
- Current `test_after.log` for backend subset proof

Actions:

- Run the supervisor-delegated backend proof command.
- Run a representative allowlist including at least the source-idea examples:
  `src/20001026-1.c`, `src/20050215-1.c`, `src/20090113-2.c`,
  `src/20090113-3.c`, and `src/20101013-1.c`.
- Confirm the generic unsupported fragment pinned in Step 2 is gone for those
  rows, or record the exact downstream owner if the route was rerouted.
- Record any new residual diagnostic and downstream owner in `todo.md`.
- If nearby `integer_div_rem` rows still retain the same owned diagnostic, do
  not accept the slice until Step 3 is generalized or ownership is corrected.

Completion check:

- `todo.md` records backend proof, representative allowlist proof, old
  diagnostic removal, and any concrete downstream residual owner.

## Step 5: Closure Check And Residual Routing

Goal: Decide whether the source idea is complete or needs a narrowed repair
packet.

Actions:

- Verify Step 1's existing div/rem opcode coverage remains in place.
- Verify representative rows no longer fail for the Step 2 pinned lowering gap,
  or have been routed to concrete downstream owners.
- Compare remaining residuals against existing open ideas before creating new
  source ideas.
- If remaining same-family rows still fail with the old gap, keep the plan
  active and write the next packet in `todo.md`.
- If the source criteria are satisfied, request plan-owner closure through the
  supervisor.

Completion check:

- The source idea can be closed only when generalized div/rem lowering is
  proven and residuals, if any, are assigned to concrete downstream owners.
