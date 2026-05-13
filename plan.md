# AArch64 First Machine Node Selection Slice

Status: Active
Source Idea: ideas/open/215_aarch64_first_machine_node_selection_slice.md

## Purpose

Implement the first conservative AArch64 structured machine-instruction-node
selection slice over already accepted target-MIR records.

## Goal

Prove a small end-to-end path from prepared target-MIR records to inspectable
AArch64 machine instruction nodes without using assembly text, encoding, object
writing, or linking as the semantic handoff.

## Core Rule

This plan may construct structured AArch64 machine instruction nodes for a
narrow accepted subset, but it must not print `.s`, parse assembly, encode
instructions, write objects, link, or bypass target-MIR records.

## Read First

- `ideas/open/215_aarch64_first_machine_node_selection_slice.md`
- `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`
- `src/backend/mir/aarch64/ALLOCATION_CONTRACT.md`
- `src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `src/backend/mir/aarch64/codegen/alu.md`
- `src/backend/mir/aarch64/codegen/cast_ops.md`
- `src/backend/mir/aarch64/codegen/comparison.md`
- `src/backend/mir/aarch64/codegen/memory.md`
- `src/backend/mir/aarch64/codegen/records.md`
- existing focused AArch64 backend tests under `tests/backend/`

## Current Targets

- a minimal structured machine-node representation or selection surface under
  the existing AArch64 MIR/codegen/module boundaries
- scalar integer ALU and simple cast records accepted by prior AArch64 target
  MIR work
- representative branch/compare records accepted by prior AArch64 branch work
- spill/reload pseudo records and memory operands accepted by ideas 210 and 213
- focused backend tests that inspect structured nodes directly
- roadmap markdown that distinguishes target-MIR records, machine nodes, and
  future assembly printer or encoder consumers

## Non-Goals

- Do not emit assembly text, parse assembly, encode object bytes, write ELF, or
  link.
- Do not implement full AArch64 instruction selection.
- Do not add final call, return, prologue, epilogue, variadic, FP, vector,
  atomics, intrinsic, inline-asm, or linker-sensitive coverage unless already
  required by the narrow selected subset.
- Do not implement register allocation, register scavenging, frame layout, or
  stack slot computation.
- Do not use testcase-shaped shortcuts or named-fixture recognition as the
  selection rule.

## Working Model

The accepted handoff for this plan is:

```text
PreparedBirModule / accepted AArch64 target-MIR records
  -> selected AArch64 machine instruction nodes
  -> proof via structured records and focused backend tests
```

Machine nodes should preserve opcode or family identity, typed operands, source
provenance, def/use/clobber data, and side-effect metadata required by
`MACHINE_INSTRUCTION_NODE_CONTRACT.md`.

Unsupported target-MIR records must fail closed with clear diagnostics or an
explicit deferred status. Missing source facts should become separate
`ideas/open/` work instead of AArch64-local guesses.

## Execution Rules

- Start with inspection. Do not create selection code until the accepted record
  inputs and existing machine-node contract gaps are named in `todo.md`.
- Select from target-MIR records only. Do not read legacy assembly strings,
  rendered BIR text, old examples, or test fixture names as semantic input.
- Keep the first subset RISC-like and small: scalar ALU/cast, branch/compare,
  and spill/reload/load/store operands only where the accepted records already
  support them.
- Consume physical registers, reserved scratch authority, spill slot ids,
  prepared provenance, and memory operands from accepted records.
- For code-changing steps, run a fresh build plus the supervisor-selected
  backend proof. Use broader validation if the selection surface crosses
  multiple backend buckets.
- Docs-only steps need stale-claim searches plus `git diff --check`.

## Steps

### Step 1: Inspect Existing Machine Node Selection Inputs

Goal: identify the accepted target-MIR records, machine-node contract surface,
and test hooks available for the first selection slice.

Primary target:
`src/backend/mir/aarch64/` records, contracts, and focused backend tests.

Actions:

- Inspect scalar ALU, cast, branch/compare, memory operand, allocation, and
  spill/reload record surfaces.
- Inspect `MACHINE_INSTRUCTION_NODE_CONTRACT.md` for required node identity,
  operand, def/use, clobber, side-effect, and provenance metadata.
- Identify whether any machine-node representation already exists or whether a
  minimal new one is required.
- Identify unsupported records that should fail closed instead of being
  silently skipped.
- Record the first safe subset and any missing facts in `todo.md`.

Completion check:
`todo.md` records the concrete first subset, the selected code/test surfaces,
and any deferred or missing-carrier findings without implementation edits.

### Step 2: Add Minimal Machine Node Model

Goal: create or tighten the smallest structured node model needed by the first
selection subset.

Primary target:
the existing AArch64 MIR/codegen/module boundary selected by Step 1.

Actions:

- Add minimal opcode or family enums and node variants for the accepted subset.
- Represent typed operands without string assembly operands.
- Preserve source provenance from the originating target-MIR records.
- Publish def/use/clobber and side-effect metadata required by the machine-node
  contract.
- Add fail-closed status or diagnostics for unsupported records.

Completion check:
The node model can represent the Step 1 subset directly and cannot be mistaken
for assembler text, encoder input, or final object output.

### Step 3: Select Scalar And Branch Nodes

Goal: lower a representative scalar/compare/branch subset from target-MIR
records into structured machine nodes.

Primary target:
scalar ALU, simple cast, compare, and branch records already accepted by prior
AArch64 target-MIR ideas.

Actions:

- Select representative integer scalar ALU nodes from accepted scalar records.
- Select simple cast nodes only where the target-MIR record already carries the
  needed semantics.
- Select branch and compare nodes from accepted branch/compare records.
- Preserve labels, predicates, operand widths, physical register references,
  and value provenance.
- Fail closed for unsupported scalar, cast, branch, or compare forms.

Completion check:
Focused tests inspect structured scalar and branch machine nodes, including
opcode/family identity, typed operands, provenance, def/use metadata, and
unsupported-case behavior.

### Step 4: Select Spill Reload And Memory Operand Nodes

Goal: lower the narrow spill/reload and memory-operand subset required for the
first structured machine-node path.

Primary target:
spill/reload records from idea 213 and memory operand records from idea 210.

Actions:

- Consume spill/reload pseudo semantics from target-MIR records without
  selecting final frame materialization or computing stack offsets locally.
- Preserve prepared spill/reload provenance, value id, register class, scratch
  authority, occupied scratch registers, slot id, and prepared offset snapshots.
- Use typed memory operands only where the accepted records already publish
  them.
- Fail closed for memory forms that still require future frame, address,
  global, call, or linker work.

Completion check:
Focused tests inspect representative spill and reload machine nodes with
structured scratch, slot, memory operand, provenance, and side-effect metadata.

### Step 5: Reconcile Documentation And Roadmap Boundaries

Goal: update AArch64 markdown consumers so the new selected machine nodes sit
between target-MIR records and future printer/encoder work.

Primary target:
`src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md` and relevant
`src/backend/mir/aarch64/codegen/*.md`.

Actions:

- Document that selected machine nodes are structured internal output, not
  assembly text.
- Keep `.s` printer, assembler, encoder, object writer, linker, and external
  smoke validation assigned to later ideas.
- Remove or correct stale wording that implies target-MIR records are the final
  pre-assembly structure after this slice lands.
- Split separate open ideas only for distinct missing facts or consumers that
  block the accepted subset.

Completion check:
Docs consistently distinguish target-MIR records, selected machine nodes,
future assembly printer work, and future encoder/object/linker work.

### Step 6: Add Focused Proof

Goal: prove the selected subset through structured-node inspection and a fresh
backend build/test path.

Primary target:
repo-native focused backend tests and the supervisor-selected backend subset.

Actions:

- Add or update tests that construct or derive representative machine nodes
  from accepted target-MIR records.
- Assert opcode/family identity, operands, provenance, def/use/clobber,
  side-effect metadata, and fail-closed unsupported behavior.
- Run the delegated build and backend test command.
- Escalate to broader validation if the final diff changes shared prepared
  data, common codegen contracts, or multiple backend buckets.

Completion check:
Fresh proof supports the first machine-node selection subset without assembly
text, expectation downgrades, or named-case-only shortcuts.
