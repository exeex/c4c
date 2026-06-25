# RV64 Object Route With BIR-Owned CFG and Shared Encoding

Status: Active
Source Idea: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md
Activated After: ideas/closed/359_bir_prepared_object_consumer_contract_completion.md

## Purpose

Resume RV64 object-route repair now that the shared prepared object consumer
contract is in place.

## Goal

Repair the RV64 `--codegen obj` route for representative multi-block prepared
functions without moving CFG semantics into MIR, the assembler, or the object
writer.

## Core Rule

BIR owns semantic CFG and data-flow edges. RV64 target lowering preserves that
structure into target blocks and instructions. MIR owns target representation
and pseudo lowering. The assembler/object layer owns bytes, labels, fixups,
relocations, and ELF construction, not high-level control-flow recovery.

## Read First

- `ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md`
- `ideas/closed/359_bir_prepared_object_consumer_contract_completion.md`
- The current RV64 prepared object emission diagnostics and tests.
- Existing `c4c-as`, `c4c-objdump`, inline asm, and RV64 object tests before
  changing encoding or object writer behavior.

## Current Targets

- Representative RV64 multi-block object-route cases:
  - `src/20000113-1.c`
  - `src/20000205-1.c`
  - `src/20010119-1.c`
  - `src/20030216-1.c`
- The boundary between prepared BIR CFG, RV64 target block lowering, MIR
  pseudo lowering, and RV64 object emission.
- Shared RV64 instruction encoding, labels, fixups, relocations, and ELF
  object construction where that avoids duplicate direct encoding paths.
- Pre-existing RV64 fixup/relocation/label paths that must be audited before
  this idea can close:
  - `RiscvObjectFixup` / `RiscvEncodedFragment` /
    `build_rv64_text_object_module` in the RV64 object-emission path,
  - `c4c-as.cpp::resolve_local_control_flow_labels`,
  - `src/backend/mir/riscv/assembler/elf_writer.cpp` minimal/pending
    relocation path.

## Non-Goals

- Do not add full RV64 data-section/global/string support.
- Do not implement every missing scalar opcode by testcase name.
- Do not move CFG ownership out of BIR.
- Do not make the assembler parse or recover semantic control flow.
- Do not treat textual assembly as a semantic IR.
- Do not require the heavy gcc torture scan in default full CTest.
- Do not fold broad ABI or width cleanup into this idea unless it is required
  to prove the representative object-route architecture.
- Do not leave a second production RV64 fixup/relocation path unreviewed when
  closing this idea.

## Working Model

```text
C/C++ frontend
  -> HIR
  -> BIR owns CFG, semantic control flow, and data-flow edges
  -> RV64 target route preserves BIR blocks/edges while selecting target ops
  -> MIR carries target blocks/instructions and lowers target pseudos
  -> encoder/internal assembler emits bytes, labels, fixups, relocations
  -> ELF object
```

## Execution Rules

- Start with a fresh representative-case audit against the now-shared prepared
  consumer contract.
- Diagnose which layer rejects each case before repairing that layer.
- Keep target-owned repairs hook-shaped and semantic-boundary preserving.
- RV64 object emission must consume the shared prepared object traversal event
  stream; do not manually scan `control_flow.parallel_copy_bundles` to
  reconstruct block-entry or pre-terminator copies.
- Use shared prepared-object classifiers and diagnostics where available,
  especially `make_prepared_object_function_traversal()`,
  `classify_prepared_object_move_bundle_consumer(event)`, and
  `collect_unplaced_prepared_object_parallel_copy_obligations()`.
- Reuse assembler-backed encoding/fixup/relocation machinery where possible.
- Preserve inline asm `.insn.d` and `.insn r` object behavior.
- Reject filename-shaped shortcuts, expectation weakening, target-local CFG
  reconstruction, and silent block/edge dropping.

## Step 1: Re-audit Representative RV64 Object Route Rejections

Goal: identify the current first rejecting layer for each representative case.

Concrete actions:

- Run the narrow representative allowlist under the current RV64 object route.
- For each case, record whether rejection happens in:
  - BIR/prepared-BIR shape,
  - RV64 target block preservation,
  - MIR pseudo lowering,
  - object emission,
  - labels/fixups/relocations,
  - instruction/ABI support.
- Use the shared prepared consumer diagnostics added by idea 359 where
  applicable.
- Record the audit table in `todo.md` before implementation.

Completion check:

- `todo.md` names the first rejecting layer for each representative case.
- The next repair packet has one target-owned blocker and a narrow proof
  command.

## Step 2: Define the RV64 Target-Block Object Stream Boundary

Goal: make the object-emission input contract explicit before removing gates.

Concrete actions:

- Inspect the current RV64 prepared object module and object-emission builder
  surfaces.
- Identify the canonical target-block/instruction stream that object emission
  should consume.
- Define how labels, branch references, call references, and relocation needs
  flow into emission without reconstructing CFG.
- Add or update focused tests for boundary diagnostics where needed.

Completion check:

- RV64 object emission has a documented and tested target-block stream
  boundary.
- The boundary keeps CFG semantics in BIR/prepared and target-block structure
  in the RV64 route.

## Step 3: Wire RV64 Object Emission to the Shared Traversal Stream

Goal: make RV64 object emission consume the shared prepared-object traversal
event stream before any target-local fragment repair resumes.

Concrete actions:

- Start from `make_prepared_object_function_traversal()` as the ordered source
  of labels, block-entry copies, instructions, pre-terminator/edge copies, and
  terminators.
- For each traversal move event, use
  `classify_prepared_object_move_bundle_consumer(event)` instead of scanning
  `control_flow.parallel_copy_bundles` or re-finding bundles in RV64 target
  code.
- Use `collect_unplaced_prepared_object_parallel_copy_obligations()` as the
  fail-closed diagnostic path for move obligations the shared traversal cannot
  place.
- Consume shared select, value-home, frame, and move diagnostics/classifiers
  where they exist before adding RV64-specific target evidence.
- Preserve block labels, branch edges, and target instruction order from the
  traversal stream; RV64 must not reconstruct CFG semantics from prepared
  state.
- Keep unsupported shapes diagnostic-specific rather than collapsing them into
  coarse prepared-module-shape failure.
- Do not claim `src/20000113-1.c` progress from the reverted/stashed
  target-local route.

Completion check:

- RV64 object emission has a shared-traversal-first integration point.
- No new RV64 code manually scans `control_flow.parallel_copy_bundles` to
  reconstruct block-entry or pre-terminator copies.
- Invalid or unsupported traversal events fail closed through shared
  diagnostics plus RV64-specific evidence where needed.
- Any later representative progress is reproven from the accepted route, not
  inherited from the stashed target-local commits.

## Step 4: Audit RV64 Fixup, Relocation, and Label Ownership

Goal: classify every pre-existing RV64 fixup/relocation/label path before
sharing or extending any machinery.

Concrete actions:

- Audit `RiscvObjectFixup`, `RiscvEncodedFragment`, and
  `build_rv64_text_object_module` in the RV64 object-emission path. Classify
  which responsibilities are valid low-level object concerns: bytes, ELF
  relocations, symbol references, range checks, and final section placement.
- Audit `c4c-as.cpp::resolve_local_control_flow_labels`. Decide explicitly
  whether it remains a textual-assembly local-label parser concern or should
  share lower-level label/fixup machinery with object emission.
- Audit `src/backend/mir/riscv/assembler/elf_writer.cpp`, including its
  minimal historical ELF/fixup path and pending relocation structures. Decide
  whether it is dead/legacy, assembler-local, or a duplicate production path
  that must be unified/removed before closure.
- Classify each path as exactly one of:
  - low-level object concern,
  - textual assembler local-label concern,
  - duplicate path to unify or remove,
  - misplaced prepared/BIR semantic reconstruction.
- If the missing piece belongs to prepared traversal, move/select/value-home,
  frame, or diagnostics, stop and route it to a 359 follow-up/reopen instead
  of patching RV64/MIR locally.
- Only after the audit, reuse or unify `c4c-as` / internal assembler encoding
  and range-checking machinery where the current architecture supports it.
- Keep assembler responsibilities limited to machine encoding, label offset
  fixups, immediate checks, and relocation records.

Completion check:

- `todo.md` contains an ownership table for the three concrete surfaces above.
- No unreviewed second production fixup/relocation path remains in scope for
  356 closure.
- Any path that reconstructs CFG, edge-copy placement, or prepared data-flow
  semantics is routed back to a 359 follow-up/reopen before 356 continues.
- RV64 object emission can encode representative labels and branch/call
  relocations from the target-block stream.
- Existing assembler, objdump, inline asm, and curated RV64 object tests remain
  green.

## Step 5: Prove Representative Executable Progress

Goal: show real RV64 object-route progress without testcase overfit.

Concrete actions:

- Run the representative allowlist proof:

```sh
CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-multiblock-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

- Include representative cases:
  - `src/20000113-1.c`
  - `src/20000205-1.c`
  - `src/20010119-1.c`
  - `src/20030216-1.c`
- Run affected backend/unit baselines for touched surfaces.
- If a milestone is reached, run:

```sh
CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Completion check:

- The implementation documents which layer rejected each representative case
  before repair.
- The object route no longer rejects representative single-module multi-block
  functions solely due to prepared block count when valid target blocks exist.
- The full gcc torture backend scan shows a real reduction in
  `multi_block_control_flow` or its successor structured diagnostic bucket
  before this idea is closed.
- The Step 4 fixup/relocation/label ownership audit is complete, with
  accepted low-level object responsibilities separated from textual assembler
  local-label parsing, duplicate paths to unify/remove, and misplaced
  BIR/prepared semantic reconstruction.

## Validation Ladder

- Audit/lifecycle-only packets: no build required.
- Code/test packets: run `cmake --build --preset default` and the focused
  backend/unit tests for touched surfaces.
- Representative object-route packets: run the allowlisted gcc torture backend
  proof with `CASE_TIMEOUT_SEC=20`.
- Milestone or closure packets: run affected baselines plus the full RV64 gcc
  torture backend scan.
- Closure is blocked until the Step 4 RV64 fixup/relocation/label ownership
  audit is recorded and all duplicate or misplaced production paths have an
  accepted resolution.

## Reviewer Reject Signals

- Reject fixes that special-case the listed torture filenames.
- Reject unrelated one-off instruction snippets that leave duplicated object
  route architecture unresolved.
- Reject MIR, assembler, or object-emission code that reconstructs CFG
  semantics that should come from BIR.
- Reject asking the assembler to understand loops, switches, if/else
  structure, liveness, or inline-asm operand relationships.
- Reject labels or relocations that work only for one known CFG shape.
- Reject closing 356 while a second production RV64 fixup/relocation path
  exists unreviewed.
- Reject using RV64 fixups/relocations to hide CFG, edge-copy placement,
  select-carrier, value-home, frame, or diagnostic semantics that belong to
  the shared prepared-object contract from 359.
- Reject treating `c4c-as.cpp::resolve_local_control_flow_labels` or the
  `riscv/assembler/elf_writer.cpp` minimal/pending relocation path as
  production object-route machinery without explicit ownership review.
- Reject routes that silently drop blocks, calls, declarations, or branch
  edges to make object emission succeed.
- Reject proof based on expectation weakening instead of executable RV64
  link-and-qemu correctness.
