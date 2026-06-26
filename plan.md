# RV64 Object Route Instruction Fragment Lowering Runbook

Status: Active
Source Idea: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md

## Purpose

Turn the current RV64 prepared-object `unsupported_instruction_fragment` bucket
into reusable target lowering work that reduces real gcc_torture backend
failures without weakening contracts.

## Goal

Repair prepared BIR instruction fragments that already crossed the semantic
handoff and are rejected inside RV64 object emission.

## Core Rule

Lower reusable prepared instruction families. Do not add filename-specific
shortcuts, expectation downgrades, allowlist filtering, or BIR control/data-flow
reconstruction inside the RV64 object emitter.

## Read First

- `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`
- `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
- Representative case:
  `tests/c/external/gcc_torture/src/20000223-1.c`
- RV64 gcc_torture backend runner:
  `scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- Object-route diagnostics and RV64 prepared-object lowering code in `src/`

## Current Scope

- Classify the 385-case `unsupported_instruction_fragment` bucket from the
  2026-06-26 reopened classification.
- Identify concrete prepared instruction fragment families and choose a small
  representative set, starting with `src/20000223-1.c`.
- Implement semantic RV64 object lowering for reusable instruction families.
- Prove representative and nearby same-fragment cases through object emission,
  link, and qemu comparison when runnable.

## Non-Goals

- Do not rewrite gcc_torture expectations or supported/unsupported markers.
- Do not treat semantic `lir_to_bir` failures as this bucket.
- Do not perform a broad assembler/object architecture rewrite unrelated to the
  observed unsupported instruction fragments.
- Do not claim progress from a green single representative if identical nearby
  fragment failures remain unexamined.

## Working Model

- The prepared-object contract owns semantic BIR facts before target lowering.
- This plan owns RV64 object lowering for prepared instruction fragments that
  already have enough contract facts to emit machine code.
- If investigation proves a fragment lacks producer-side facts, record that in
  `todo.md` and ask the supervisor to split a separate source idea instead of
  hiding the producer issue in target emission.

## Execution Rules

- Keep each implementation packet centered on one fragment family.
- Use diagnostics and prepared dumps to group failures by semantic fragment, not
  by testcase filename.
- Preserve existing diagnostics for unsupported forms until real lowering is
  added.
- For each code-changing packet, run the supervisor-selected build/proof
  command exactly and record results in `todo.md`.
- Escalate to a refreshed subset or regression guard when lowering touches
  shared instruction selection, relocation, or runtime-sensitive value moves.

## Ordered Steps

### Step 1: Classify Instruction Fragments

Goal: identify the concrete prepared instruction fragment families behind the
`unsupported_instruction_fragment` bucket.

Primary targets:

- Existing RV64 gcc_torture scan artifacts under `build/agent_state/` and
  `build/rv64_gcc_c_torture_backend/`, if present
- Representative case `tests/c/external/gcc_torture/src/20000223-1.c`
- Prepared BIR/object-route diagnostic output

Actions:

- Load or regenerate enough scan data to reproduce the current representative
  failure.
- Extract cases with
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`.
- Group failing cases by prepared instruction opcode, operand/home shapes, and
  required RV64 emission behavior.
- Select representative and nearby same-fragment cases for the first repair
  packet.
- Record classification notes and proof commands in `todo.md`.

Completion check:

- `todo.md` names the first fragment family, representative cases, observed
  diagnostics, and the narrow proof command the executor should use next.

### Step 2: Lower The First Reusable Fragment Family

Goal: add RV64 object lowering for the highest-value classified instruction
fragment family that has sufficient prepared contract facts.

Primary targets:

- RV64 prepared-object instruction lowering implementation in `src/`
- Existing tests or gcc_torture runner coverage for the selected fragment

Actions:

- Inspect the existing RV64 lowering path for adjacent supported instruction
  fragments.
- Add the minimal semantic lowering rule for the selected family.
- Preserve explicit rejection diagnostics for still-unsupported operand shapes.
- Avoid copying BIR producer logic into the target emitter.

Completion check:

- The selected representative no longer fails with
  `unsupported_instruction_fragment`, and nearby same-fragment cases are either
  fixed or explicitly separated by a different diagnostic/root cause.

### Step 3: Prove Runtime And Bucket Reduction

Goal: verify that newly emitted object code is correct enough to reduce the
bucket without introducing runtime regressions.

Primary targets:

- Representative and nearby same-fragment cases
- Supervisor-selected temporary allowlist or backend scan subset
- Canonical `test_after.log` when delegated by the supervisor

Actions:

- Run the delegated build/proof command for the changed fragment family.
- Prefer qemu comparison proof for cases that compile and link.
- Refresh a subset large enough to show the fragment bucket count decreases.
- Record any new runtime mismatch or producer-side missing fact in `todo.md`.

Completion check:

- Proof logs show reduced `unsupported_instruction_fragment` failures for the
  selected family and no new runtime abort, segfault, or comparison mismatch in
  the proved subset.

### Step 4: Iterate Or Split Remaining Families

Goal: continue with the next classified fragment family or split work that is
not target-lowering scope.

Primary targets:

- Remaining classified instruction fragment families
- Any producer-side missing fact discovered during lowering

Actions:

- Pick the next reusable target-lowering family only after the previous packet
  has proof recorded in `todo.md`.
- Ask the supervisor for a new source idea when a remaining family needs
  upstream BIR/prepared-contract repair.
- Keep routine progress in `todo.md`; rewrite this plan only for a real route
  reset or scope split.

Completion check:

- The dominant repairable instruction fragment families are lowered or split
  into precise follow-up ideas, and refreshed proof shows the 385-case bucket is
  materially reduced without testcase overfit.
