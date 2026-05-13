# BIR MIR Allocation Contract Runbook

Status: Active
Source Idea: ideas/open/212_bir_mir_allocation_contract.md

## Purpose

Define and apply the allocation boundary between BIR / prepared facts and
target MIR codegen before AArch64 machine instruction node emission.

Goal: make register budgets, physical-register homes, spill slots, reserved
MIR scratch use, and future virtual-register placeholders explicit contract
data instead of ad hoc choices made by individual lowering slices.

## Core Rule

Do not make core BIR, rendered register names, assembly text, or final stack
offset strings the authority for target allocation. Allocation authority lives
in a structured BIR/prepared-to-MIR contract, with later lowering consuming
that result.

## Read First

- ideas/open/212_bir_mir_allocation_contract.md
- ideas/closed/211_aarch64_machine_instruction_node_contract.md
- src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md
- src/backend/mir/aarch64/abi/
- src/backend/mir/aarch64/module/
- src/backend/mir/aarch64/codegen/records.hpp
- src/backend/mir/aarch64/codegen/records.cpp
- src/backend/mir/aarch64/codegen/*.md
- src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md
- src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md
- src/backend/mir/aarch64/CLASSIFICATION_INDEX.md

## Current Targets

- A committed AArch64 markdown allocation contract under
  `src/backend/mir/aarch64/`.
- Implemented AArch64 ABI, module, and codegen record surfaces that mention
  target registers, physical names, caller/callee-save behavior, scratch
  registers, or spill ownership.
- Not-yet-implemented AArch64 markdown artifacts that currently imply each
  feature slice should allocate registers, invent spills, or patch calling
  convention facts opportunistically.
- Separate open gap ideas for missing shared BIR/prepared carriers if the
  allocation boundary cannot be represented by existing facts.

## Non-Goals

- Do not implement a full register allocator.
- Do not implement an x86/x86-64 backend or CISC-specific optimizations.
- Do not implement frame layout, concrete stack offsets, prologue/epilogue
  emission, or callee-save save/restore instruction emission.
- Do not emit reload/store machine instructions.
- Do not complete calls, returns, variadics, inline asm, atomics, or vector
  lowering.
- Do not change parser, sema, HIR, or core BIR semantics inside this plan.
- Do not use assembly text, rendered names, or parsed `.s` as allocation
  authority.

## Working Model

```text
BIR / PreparedBirModule
  -> target ABI register-budget contract
  -> allocation result
  -> AArch64 target MIR records
  -> AArch64 machine instruction nodes
  -> .s printer / encoder
```

The allocation result maps prepared/BIR values to target physical registers,
structured spill slots, future virtual-register placeholders, or existing
non-register prepared locations. Spill slot ids stay abstract until frame
layout assigns concrete offsets.

## Execution Rules

- Keep source idea edits rare; routine audit and packet notes belong in
  `todo.md`.
- Prefer a markdown contract first, then narrowly align implemented surfaces
  only where they currently hide allocation ownership.
- Separate GPR and FPR/SIMD register classes throughout the contract.
- Treat reserved MIR scratch registers as unavailable for long-lived BIR value
  homes.
- Preserve x86/x86-64 correctness requirements in the portable contract even
  though this plan does not implement an x86 backend.
- If a missing BIR/prepared carrier is required, create a separate
  `ideas/open/` gap idea instead of patching around it in AArch64.
- Code-changing steps require fresh build or compile proof before acceptance.
- Reject expectation downgrades, named-case shortcuts, or classification-only
  changes claimed as allocation capability progress.

## Ordered Steps

### Step 1: Audit Existing Allocation Surfaces

Goal: identify implemented and markdown surfaces that currently own or imply
target register allocation, spill policy, scratch use, or calling-convention
resource decisions.

Primary targets:
- src/backend/mir/aarch64/abi/
- src/backend/mir/aarch64/module/
- src/backend/mir/aarch64/codegen/records.hpp
- src/backend/mir/aarch64/codegen/records.cpp
- src/backend/mir/aarch64/codegen/*.md
- src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md
- src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md
- src/backend/mir/aarch64/CLASSIFICATION_INDEX.md

Actions:
- Inspect implemented `.cpp/.hpp` files for rendered physical-register names,
  implicit spill ownership, scratch-register assumptions, or target-local
  allocation decisions.
- Inspect markdown artifacts for future-direction language that scatters
  allocation across scalar ALU, memory, branch, call, return, or vector
  slices.
- Record concrete conflict files and any missing shared-carrier concerns in
  `todo.md` before implementation edits begin.

Completion check:
- `todo.md` names the implemented surfaces and markdown artifacts that need
  allocation-boundary alignment, with no broad source idea rewrite.

### Step 2: Commit The Allocation Contract

Goal: add the canonical AArch64 allocation-boundary contract document.

Primary target:
- src/backend/mir/aarch64/

Actions:
- Add or update a markdown contract under `src/backend/mir/aarch64/` for the
  BIR/prepared-to-MIR allocation boundary.
- Define target ABI register budgets and register classes, including separate
  GPR and FPR/SIMD pools.
- Define allocatable long-lived, argument/return, caller-saved temp, reserved
  MIR scratch, and special/forbidden pools.
- Define physical-register allocation results, structured spill-slot ids, and
  future virtual-register placeholders.
- Define call-clobber and callee-save obligations without implementing
  prologue/epilogue emission.
- Define where reload/store pseudo operations or equivalent structured
  lowering plans are introduced.
- Include x86/x86-64 correctness carriers for fixed/implicit operands, flags,
  subregister aliases, width effects, ABI stack resources, and target ABI
  variants.

Completion check:
- The contract separates allocation from machine instruction nodes, states
  finite physical-register budgets, and states logically unbounded spill-slot
  assignment.

### Step 3: Align Implemented AArch64 Surfaces

Goal: make currently implemented ABI, module, and record surfaces explicit
about whether they consume, expose, or avoid allocation ownership.

Primary targets:
- src/backend/mir/aarch64/abi/
- src/backend/mir/aarch64/module/
- src/backend/mir/aarch64/codegen/records.hpp
- src/backend/mir/aarch64/codegen/records.cpp

Actions:
- Add narrow names, comments, enums, or record fields only where needed to
  prevent ad hoc register/spill ownership.
- Preserve existing prepared-id and target-record provenance.
- Do not introduce full allocation algorithms, frame offsets, reload/store
  instruction emission, or machine-node opcode expansion.
- Keep reserved MIR scratch registers distinct from long-lived allocation
  registers.

Completion check:
- Implemented surfaces either reference the allocation contract or encode the
  same boundary directly without making assembly text or rendered names the
  allocation authority.
- Fresh build or compile proof covers touched `.cpp/.hpp` files.

### Step 4: Align AArch64 Markdown Roadmap Artifacts

Goal: make future AArch64 implementation notes route allocation-sensitive
work through the shared allocation result contract.

Primary targets:
- src/backend/mir/aarch64/codegen/*.md
- src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md
- src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md
- src/backend/mir/aarch64/CLASSIFICATION_INDEX.md
- Other AArch64 markdown artifacts discovered in Step 1.

Actions:
- Replace language that lets each feature slice allocate registers or invent
  spill slots independently.
- Route long-lived value homes through the allocation result.
- Route spill/reload materialization through reserved MIR scratch policy and
  later target MIR pseudo operations or structured lowering plans.
- Keep historical inventories intact where useful, but make future-direction
  guidance consistent with the allocation boundary.

Completion check:
- Markdown artifacts no longer direct future work to scatter allocation,
  spill, scratch, or calling-convention ownership across individual lowering
  slices.

### Step 5: Capture Shared Carrier Gaps

Goal: ensure missing BIR/prepared representation needs become durable separate
initiatives instead of target-local workarounds.

Actions:
- Review audit notes and implementation alignment for missing allocation input
  or output carriers.
- If a missing carrier is required, create a separate `ideas/open/` gap idea
  with concrete acceptance criteria and reviewer reject signals.
- If no missing carrier blocks this contract, record that conclusion in
  `todo.md`.

Completion check:
- Any required shared BIR/prepared carrier gap is represented as its own open
  idea, or `todo.md` explicitly states that no separate gap was found.

### Step 6: Add Focused Proof

Goal: prove the allocation contract and implemented-surface alignment without
expanding backend capability scope.

Primary targets:
- Existing AArch64 backend compile/test bucket chosen by the supervisor.
- Focused ABI, module, or record tests if an appropriate local pattern exists.

Actions:
- Add focused tests only if the repository already has a narrow pattern for
  the touched surfaces.
- Otherwise run compile/build proof that exercises touched implemented files.
- Do not downgrade expectations or claim capability progress through
  classification-only changes.

Completion check:
- `test_after.log` or supervisor-chosen proof records the exact command and
  result.
- The proof covers representative ownership of allocation-boundary data from
  existing AArch64 surfaces.

### Step 7: Final Consistency Review

Goal: verify the active plan satisfies the source idea acceptance criteria.

Actions:
- Compare the diff against the source idea and reject routes that move
  allocation authority into core BIR, assembly text, rendered physical names,
  or final stack offsets.
- Confirm GPR and FPR/SIMD pools remain separate.
- Confirm reserved MIR scratch registers are excluded from long-lived
  allocation homes.
- Confirm the contract can represent x86/x86-64 fixed operands, flags,
  subregister aliases, and ABI stack resources without implementing x86.
- Confirm no full allocator, frame layout, concrete reload/store emission,
  parser/sema/HIR/core-BIR rewrite, or idea-211 assembler/encoder handoff work
  slipped into the slice.

Completion check:
- All acceptance criteria in the source idea are satisfied or explicit
  leftover work is recorded for lifecycle judgment.
