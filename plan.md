# AArch64 Scalar Machine-Node Operand Fact Printing Runbook

Status: Active
Source Idea: ideas/open/334_aarch64_scalar_machine_node_operand_fact_printing.md
Split from: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the current AArch64 compile/printer failures where selected scalar
machine nodes reach the printer without coherent operand facts for spilled,
frame-slot, or memory-backed operands.

## Goal

Make selected AArch64 scalar arithmetic nodes printable by preserving or
materializing structured operand facts for scalar operands across preparation,
selection, register allocation, spill/reload, and machine printing.

## Core Rule

Repair a general scalar machine-node operand fact capability. Do not
special-case `00164.c`, `00214.c`, one function, one instruction index, one
stack offset, one scalar opcode, one register, or one emitted instruction
sequence.

## Read First

- `ideas/open/334_aarch64_scalar_machine_node_operand_fact_printing.md`
- `todo.md`
- `test_after.log`
- generated artifacts for `00164.c` and `00214.c` under
  `build/c_testsuite_aarch64_backend/src/`
- AArch64 scalar ALU, scalar mul/div/rem, operand resolution, instruction
  record, and machine-printer code
- existing focused AArch64 scalar, memory operand, frame-slot, and printer
  guardrails

## Current Targets

- `c_testsuite_aarch64_backend_src_00164_c`
  - current first bad fact: scalar `mul` printer reports incomplete printable
    RHS facts
  - representative prepared operations: `%t159 = bir.mul i32 %t157, %t158`
    and `%t168 = bir.mul i32 %t166, %t167`
- `c_testsuite_aarch64_backend_src_00214_c`
  - current first bad fact: scalar `add` printer reports missing prepared
    frame-slot sources for memory operands
- Focused backend tests that can observe selected scalar operand facts before
  c-testsuite runtime.

## Non-Goals

- Do not implement under the umbrella inventory idea 295.
- Do not reopen parked idea 316's frame-layout route unless current
  localization proves frame allocation is the first owner after, or inside,
  this scalar-printer path.
- Do not repair runtime mismatch, runtime nonzero, timeout, output-storm,
  runner, expectation, unsupported classification, CTest registration, or
  proof-log behavior here.
- Do not reopen fixed-formal, byval, HFA/floating, stdarg cursor, MOVI,
  large-offset spelling, or local/value publication owners without direct
  current generated-code evidence.

## Working Model

The backend now selects scalar machine nodes for these cases, but the AArch64
printer requires operand facts that distinguish register, immediate,
frame-slot, spill-slot, and materialized memory-backed sources. The first
packet should localize which selected record or operand fact is missing before
editing implementation.

## Execution Rules

- Start from `test_after.log`, generated assembly, prepared BIR dumps, and
  machine-node/instruction records.
- Use c4c-clang-tools before large C++ reads when localizing implementation
  owners.
- Keep routine progress in `todo.md`.
- Add or update focused backend coverage before relying on external
  c-testsuite representatives.
- If the representatives advance to frame layout, runtime mismatch, or another
  distinct owner, record the new first bad fact in `todo.md` and return to
  lifecycle classification.

## Ordered Steps

### Step 1: Localize Scalar Operand Fact Divergence

Goal: identify the first selected-machine-node or printer contract point where
operand facts become insufficient for the current representatives.

Primary targets:

- selected machine records for `00164.c` scalar `mul`
- selected machine records for `00214.c` scalar `add`
- AArch64 scalar operand resolution and printer diagnostics
- prepared storage/home records for the named operands

Actions:

- Map each failing scalar node from prepared BIR value to selected machine
  record and printer operand.
- Record source operands, prepared homes, selected operand facts, storage
  widths, and the exact missing printer fact.
- Determine whether the first owner is selection, operand resolution,
  spill/frame-slot fact propagation, rematerialization, or printer acceptance.
- Separate this owner from frame-layout/prologue allocation evidence.

Completion check:

- `todo.md` names the first scalar operand-fact owner with generated-code
  evidence for both representatives or explains why they split into separate
  owners.

### Step 2: Repair The Classified Operand-Fact Owner

Goal: make the selected scalar operand facts coherent enough for AArch64
printing and materialization.

Primary targets: implementation files identified by Step 1.

Actions:

- Apply the narrow semantic repair for the classified owner.
- Preserve scalar operation semantics, width, signedness, frame-slot identity,
  and existing register/immediate operand behavior.
- Avoid named-case or literal-offset shortcuts.

Completion check:

- The current scalar `mul` and scalar `add` printer diagnostics no longer
  occur through the repaired general path.

### Step 3: Add Focused Coverage

Goal: prove the repaired scalar operand-fact capability locally.

Primary targets: focused AArch64 backend tests for scalar instruction records,
operand resolution, machine printing, or dispatch.

Actions:

- Add coverage that fails without the repair for spilled/frame-slot scalar
  operands.
- Cover at least one scalar `mul`/`div`/`rem` RHS-fact case and one scalar
  `add`/`sub`/`bitwise` frame-slot or memory-operand case.
- Include adjacent scalar, memory operand, and printer guardrails in the
  focused proof when cheap.

Completion check:

- Focused backend tests prove the repaired operand facts and preserve adjacent
  guardrails.

### Step 4: Validate Representatives And Classify Residuals

Goal: prove the focused repair on the external representatives and classify
the next first bad fact.

Primary targets:

- `c_testsuite_aarch64_backend_src_00164_c`
- `c_testsuite_aarch64_backend_src_00214_c`

Actions:

- Run the delegated focused proof scope.
- Confirm the old printer diagnostics are gone.
- If either representative still fails, classify whether the next first bad
  fact remains in scalar operand facts, moves to frame layout, or belongs to a
  different runtime family.

Completion check:

- `todo.md` records fresh proof, the old printer diagnostics are gone, and any
  residual has a lifecycle-ready owner classification.
