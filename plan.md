# AArch64 Uninitialized Local Slot Runtime Residual Runbook

Status: Active
Source Idea: ideas/open/335_aarch64_uninitialized_local_slot_runtime_residual.md
Split from: ideas/closed/334_aarch64_scalar_machine_node_operand_fact_printing.md

## Purpose

Repair the current AArch64 runtime mismatch where generated code for
`00164.c` reads stack slots that appear uninitialized after scalar
machine-node operand fact printing was fixed.

## Goal

Make the affected AArch64 local-slot values initialized or published before
their later loads consume them, without weakening the closed scalar printer
repair.

## Core Rule

Repair the general backend initialization, publication, frame-slot,
spill/reload, or store/load sequencing owner. Do not special-case `00164.c`,
one function, one instruction index, one stack offset, one printed value, or
one emitted instruction sequence.

## Read First

- `ideas/open/335_aarch64_uninitialized_local_slot_runtime_residual.md`
- `test_after.log`
- generated artifacts for `c_testsuite_aarch64_backend_src_00164_c` under
  `build/c_testsuite_aarch64_backend/src/`
- prepared BIR, selected machine records, generated assembly, frame-slot
  assignment, local/value publication, spill/reload, and AArch64 load/store
  emission paths
- closed scalar printer context in
  `ideas/closed/334_aarch64_scalar_machine_node_operand_fact_printing.md`

## Current Targets

- `c_testsuite_aarch64_backend_src_00164_c`
  - current first visible mismatch: expected `46`, actual `-635898024` at
    `tests/c/external/c-testsuite/src/00164.c:28`
  - observed residual: generated assembly reads stack locations around
    `sp+#148`, `sp+#152`, and `sp+#156` that appear uninitialized
  - old scalar `mul` printer diagnostic is no longer the first bad fact
- Focused backend tests that can observe local-slot initialization,
  publication, spill/reload, frame-slot, or load/store sequencing before the
  external c-testsuite runtime.

## Non-Goals

- Do not reopen scalar machine-node operand fact printing unless fresh proof
  shows the closed scalar printer diagnostics returned.
- Do not reopen parked idea 316 unless localization proves frame allocation or
  slot layout is the current first owner.
- Do not repair byval, HFA/floating, stdarg cursor, MOVI, large-offset, runner,
  timeout, unsupported-classification, expectation, or CTest-registration
  behavior here without direct current evidence.
- Do not fix only a named testcase, stack offset, output value, instruction
  index, or emitted instruction spelling.

## Working Model

The scalar printer repair made the representative printable, but execution now
loads values from stack locations that do not appear to have received the
intended stores. The first packet should establish whether the missing fact is
value publication, local-slot initialization, frame-slot home assignment,
spill/reload sequencing, or generated load/store ordering before editing code.

## Execution Rules

- Start from `test_after.log`, generated assembly, prepared BIR dumps, and
  selected machine records for `00164.c`.
- Use c4c-clang-tools before large C++ reads when localizing implementation
  owners.
- Keep routine progress in `todo.md`.
- Add or update focused backend coverage before relying on the external
  c-testsuite representative.
- Preserve the repaired scalar operand-fact printer path from idea 334.
- If `00164.c` advances to a different runtime family, record the new first
  bad fact in `todo.md` and return to lifecycle classification.

## Ordered Steps

### Step 1: Localize Uninitialized Local-Slot First Bad Fact

Goal: identify the first backend owner that should have initialized, stored,
published, or reloaded the values later read from the observed stack slots.

Primary targets:

- generated assembly and dumps for `00164.c`
- stack slots around `sp+#148`, `sp+#152`, and `sp+#156`
- corresponding prepared BIR values, selected machine operands, frame-slot
  homes, spill/reload records, and load/store emissions

Actions:

- Map the first mismatched output back to the generated loads, stack slots,
  source values, and expected stores.
- Determine whether the first missing fact is value publication, local-slot
  initialization, frame-slot home assignment, spill/reload sequencing, or
  load/store ordering.
- Separate this owner from the closed scalar printer route and from parked
  frame-layout work unless current evidence proves otherwise.

Completion check:

- `todo.md` names the first owner with generated-code evidence, including the
  values, stack slots, expected stores, and actual loads involved.

### Step 2: Repair The Classified Initialization Or Publication Owner

Goal: make initialized values reach the stack homes consumed by the later
AArch64 loads.

Primary targets: implementation files identified by Step 1.

Actions:

- Apply the narrow semantic repair for the classified owner.
- Preserve value identity, storage width, signedness, frame-slot identity,
  spill/reload behavior, and existing scalar printer behavior.
- Avoid named-case, literal-offset, instruction-index, and output-value
  shortcuts.

Completion check:

- The previously uninitialized local-slot load pattern no longer occurs
  through the repaired general path.

### Step 3: Add Focused Coverage

Goal: prove the repaired local-slot initialization or publication capability
locally.

Primary targets: focused AArch64 backend tests for the owner classified in
Step 1.

Actions:

- Add coverage that fails without the repair for the missing initialization,
  publication, spill/reload, frame-slot, or load/store sequencing behavior.
- Preserve adjacent scalar arithmetic, local-slot, spill/reload, frame-slot,
  and machine-printer guardrails when cheap.

Completion check:

- Focused backend tests prove the repaired owner and preserve adjacent
  guardrails.

### Step 4: Validate Representative And Classify Residuals

Goal: prove the focused repair on `00164.c` and classify any next first bad
fact.

Primary target:

- `c_testsuite_aarch64_backend_src_00164_c`

Actions:

- Run the delegated focused proof scope.
- Confirm the old scalar printer diagnostics remain gone.
- If the representative still fails, classify whether the next first bad fact
  remains in this owner or belongs to a different runtime family.

Completion check:

- `todo.md` records fresh proof, the old scalar printer diagnostics remain
  gone, and `00164.c` either passes or has a lifecycle-ready residual
  classification.
