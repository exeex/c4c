# AArch64 Machine Instruction Node Contract Runbook

Status: Active
Source Idea: ideas/open/211_aarch64_machine_instruction_node_contract.md

## Purpose

Define and apply the AArch64 machine-instruction node contract between MIR
codegen and assembler/encoder layers.

Goal: make structured machine instruction nodes the internal semantic handoff;
assembly text is only printer/debug/external-assembler output.

## Core Rule

Do not introduce or preserve a `codegen -> asm text -> assembler parser`
semantic path. Machine instruction operands must come from BIR, prepared facts,
target MIR records, and typed AArch64 records, not rendered assembly strings.

## Read First

- ideas/open/211_aarch64_machine_instruction_node_contract.md
- src/backend/mir/aarch64/codegen/records.hpp
- src/backend/mir/aarch64/codegen/records.cpp
- src/backend/mir/aarch64/abi/
- src/backend/mir/aarch64/module/
- src/backend/mir/aarch64/codegen/*.md
- src/backend/mir/aarch64/assembler/*.md
- src/backend/mir/aarch64/assembler/encoder/*.md
- src/backend/mir/aarch64/linker/
- src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md
- src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md
- src/backend/mir/aarch64/CLASSIFICATION_INDEX.md

## Current Targets

- Implemented AArch64 `.cpp/.hpp` record surfaces that currently imply or
  should reference a machine-node pipeline.
- A committed markdown contract under `src/backend/mir/aarch64/` that defines
  the machine instruction node boundary.
- Not-yet-implemented AArch64 markdown artifacts that mention assembly text,
  assembler/parser recovery, encoder input, or linker/object expectations.
- Focused compile or test proof for representative machine-node construction
  or ownership from existing target records.

## Non-Goals

- Do not emit real assembly text for scalar, memory, branch, call, or return
  operations.
- Do not implement instruction encoding, object writing, or linking.
- Do not complete all opcode families in this plan.
- Do not rewrite the completed memory operand model beyond minimum naming or
  contract alignment.
- Do not parse assembly text to recover operands for codegen.
- Do not add peephole passes.

## Working Model

```text
BIR / PreparedBirModule
  -> AArch64 target MIR records
  -> AArch64 machine instruction nodes
  -> optional .s printer
  -> optional structured encoder / object writer
```

Target MIR records describe lowering intent and prepared facts. Machine
instruction nodes are the typed instruction handoff. The `.s` printer and any
future encoder/object writer consume structured nodes or an explicitly lower
structured encoding record.

## Execution Rules

- Keep source idea edits rare; routine execution notes belong in `todo.md`.
- Prefer small behavior-preserving code and markdown slices.
- For implemented records, align naming and data shape with structured
  machine nodes instead of creating text-emitter abstractions.
- For markdown artifacts, remove future-direction language that makes assembly
  text the backend semantic authority.
- If a missing BIR/prepared carrier blocks a typed operand, record it in
  `todo.md`; create a separate open idea only if it becomes a distinct
  initiative.
- Code-changing steps require fresh build or compile proof before acceptance.
- Escalate to broader validation if record changes affect shared ABI/module
  surfaces beyond one narrow bucket.

## Ordered Steps

### Step 1: Audit Existing AArch64 Surfaces

Goal: identify every implemented and markdown surface that currently defines,
implies, or contradicts the machine-instruction-node handoff.

Primary targets:
- src/backend/mir/aarch64/codegen/records.hpp
- src/backend/mir/aarch64/codegen/records.cpp
- src/backend/mir/aarch64/abi/
- src/backend/mir/aarch64/module/
- src/backend/mir/aarch64/codegen/*.md
- src/backend/mir/aarch64/assembler/
- src/backend/mir/aarch64/linker/

Actions:
- Inspect implemented `.cpp/.hpp` records for assembly-text ownership,
  rendered-name authority, or missing typed node terminology.
- Inspect markdown artifacts for `codegen -> assembly text -> parser`
  direction, text peephole language, and assembler/parser operand recovery.
- Record the concrete files and conflicts in `todo.md` without editing broad
  implementation surfaces during the audit.

Completion check:
- `todo.md` names the implemented record surfaces and markdown artifacts that
  need changes, with no source idea rewrite beyond activation repair.

### Step 2: Commit The Machine Instruction Node Contract

Goal: add the canonical AArch64 contract document that defines the structured
node boundary.

Primary target:
- src/backend/mir/aarch64/

Actions:
- Add or update a markdown contract under `src/backend/mir/aarch64/` for
  machine instruction nodes.
- Define opcode or instruction-family identity.
- Define typed operand variants for register, immediate, memory, symbol,
  branch target, condition/predicate, prepared value, frame slot, and data
  reference.
- Define source metadata tying nodes back to BIR and prepared facts.
- Define def/use/clobber/side-effect metadata where later passes need it.
- Explicitly separate target MIR records, machine instruction nodes, `.s`
  printer output, and encoder/object input.

Completion check:
- The contract can be cited by codegen, assembler, encoder, and linker docs
  without implying text parsing as an internal semantic handoff.

### Step 3: Align Implemented Record Surfaces

Goal: update currently implemented AArch64 records so their names and data
shape fit the machine-node contract.

Primary targets:
- src/backend/mir/aarch64/codegen/records.hpp
- src/backend/mir/aarch64/codegen/records.cpp
- src/backend/mir/aarch64/abi/
- src/backend/mir/aarch64/module/

Actions:
- Introduce or adjust typed machine-node record references where existing
  target records need a downstream handoff.
- Ensure records do not make assembly strings the primary downstream
  representation.
- Preserve existing prepared-id and target-record provenance.
- Keep changes narrow; do not add instruction selection, assembly printing, or
  encoding behavior.

Completion check:
- Representative implemented records can construct or reference structured
  machine instruction nodes without assembly-string parsing.
- Fresh build or compile proof covers the touched `.cpp/.hpp` files.

### Step 4: Align AArch64 Markdown Roadmap Artifacts

Goal: make future AArch64 implementation notes follow the structured node
pipeline.

Primary targets:
- src/backend/mir/aarch64/codegen/*.md
- src/backend/mir/aarch64/assembler/*.md
- src/backend/mir/aarch64/assembler/encoder/*.md
- src/backend/mir/aarch64/linker/*.md
- src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md
- src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md
- src/backend/mir/aarch64/CLASSIFICATION_INDEX.md

Actions:
- Replace internal semantic handoff language based on assembly text with
  machine-node ownership language.
- Place the `.s` printer as a consumer of machine nodes.
- Place any future built-in encoder as a consumer of machine nodes or a lower
  structured encoding record.
- Remove future-direction language for text peepholes, parser operand
  recovery, or assembly-string semantic recovery.

Completion check:
- Markdown artifacts consistently direct future work through the structured
  machine-node pipeline.

### Step 5: Add Focused Proof

Goal: prove the contract and implemented records are usable without widening
the backend scope.

Primary targets:
- Existing AArch64 compile/test bucket chosen by the supervisor.
- Focused record construction or ownership tests if a local test pattern
  exists.

Actions:
- Add focused tests only if the repository has an appropriate narrow pattern.
- Otherwise run compile/build proof that exercises touched implemented
  surfaces.
- Do not downgrade expectations or claim capability progress through
  classification-only changes.

Completion check:
- `test_after.log` or supervisor-chosen proof records the exact command and
  result.
- The proof covers representative construction or ownership of machine
  instruction nodes from existing target records.

### Step 6: Final Consistency Review

Goal: verify the active plan satisfies the source idea acceptance criteria.

Actions:
- Compare the diff against the source idea and reject any route that preserves
  `codegen -> asm text -> assembler parser`.
- Check that implemented records, markdown contracts, printer placement, and
  encoder placement agree.
- Confirm no broad instruction selection, assembly emission, encoding, object
  writing, linking, or peephole work slipped into the slice.
- Run the supervisor-selected broader validation if record changes touched
  shared ABI/module surfaces.

Completion check:
- All acceptance criteria in the source idea are either satisfied or explicit
  leftover work is recorded for lifecycle judgment.
