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

## Non-Goals

- Do not add full RV64 data-section/global/string support.
- Do not implement every missing scalar opcode by testcase name.
- Do not move CFG ownership out of BIR.
- Do not make the assembler parse or recover semantic control flow.
- Do not treat textual assembly as a semantic IR.
- Do not require the heavy gcc torture scan in default full CTest.
- Do not fold broad ABI or width cleanup into this idea unless it is required
  to prove the representative object-route architecture.

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

## Step 3: Replace the Blanket Multi-Block Rejection for Valid Target Blocks

Goal: stop rejecting valid lowered target-block streams solely because the
prepared function has more than one block.

Concrete actions:

- Remove or narrow the single-block gate only after the input boundary is
  explicit.
- Preserve block labels, branch edges, and target instruction order.
- Keep unsupported shapes diagnostic-specific rather than collapsing them into
  coarse prepared-module-shape failure.

Completion check:

- Representative valid multi-block streams advance past the old blanket gate.
- Invalid or unsupported object shapes still fail closed with precise
  diagnostics.

## Step 4: Share Encoding, Label, Fixup, and Relocation Machinery

Goal: avoid a second RV64 encoding path while enabling object emission for
branch/call-bearing target streams.

Concrete actions:

- Reuse `c4c-as` / internal assembler encoding and range-checking machinery
  where the current architecture allows it.
- Route labels, fixups, and relocations through shared low-level machinery
  instead of duplicating CFG or text parsing.
- Keep assembler responsibilities limited to machine encoding, label offset
  fixups, immediate checks, and relocation records.

Completion check:

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

## Validation Ladder

- Audit/lifecycle-only packets: no build required.
- Code/test packets: run `cmake --build --preset default` and the focused
  backend/unit tests for touched surfaces.
- Representative object-route packets: run the allowlisted gcc torture backend
  proof with `CASE_TIMEOUT_SEC=20`.
- Milestone or closure packets: run affected baselines plus the full RV64 gcc
  torture backend scan.

## Reviewer Reject Signals

- Reject fixes that special-case the listed torture filenames.
- Reject unrelated one-off instruction snippets that leave duplicated object
  route architecture unresolved.
- Reject MIR, assembler, or object-emission code that reconstructs CFG
  semantics that should come from BIR.
- Reject asking the assembler to understand loops, switches, if/else
  structure, liveness, or inline-asm operand relationships.
- Reject labels or relocations that work only for one known CFG shape.
- Reject routes that silently drop blocks, calls, declarations, or branch
  edges to make object emission succeed.
- Reject proof based on expectation weakening instead of executable RV64
  link-and-qemu correctness.
