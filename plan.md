# RV64 Object Route With BIR-Owned CFG and Shared Encoding

Status: Active
Source Idea: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md

## Purpose

Make the RV64 `--codegen obj` route accept valid multi-block prepared
functions without moving CFG semantics below BIR.

## Goal

Preserve BIR-owned CFG through the RV64 target route, then emit ELF object
bytes from already-lowered target blocks using shared RV64 encoding/fixup
machinery where practical.

## Core Rule

BIR owns CFG, semantic control flow, and data-flow edges. MIR and object
emission must not reconstruct `if`, `loop`, `switch`, reachability, liveness,
or inline-asm operand relationships. The assembler/encoder only emits machine
code bytes, labels, fixups, relocations, and ELF object data.

## Read First

- `ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md`
- `review/354_prepared_shape_classification.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/asm_emitter.cpp`
- `src/backend/mir/riscv/codegen/rv64_line_assembler.cpp`
- `src/backend/mir/riscv/assembler/`
- `tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`

## Current Targets

- RV64 backend object route for prepared functions.
- Representative multi-block gcc torture cases:
  - `src/20000113-1.c`
  - `src/20000205-1.c`
  - `src/20010119-1.c`
  - `src/20030216-1.c`
- Existing curated RV64 object/runtime tests and inline asm behavior.

## Non-Goals

- Do not implement data-section/global/string support here except where needed
  to classify multi-block blockers; that belongs to idea 357.
- Do not add testcase-name shortcuts or one-off filename handling.
- Do not downgrade unsupported tests or weaken runtime comparison contracts.
- Do not make the assembler parse or recover semantic control flow.
- Do not require the full gcc torture backend scan in default full CTest.
- Do not rebuild unrelated ABI, vararg, or FPR support beyond the boundary
  needed to prove the object-route architecture.

## Working Model

```text
C/C++ frontend
  -> HIR
  -> BIR owns CFG, semantic control flow, and data-flow edges
  -> RV64 target route preserves BIR blocks/edges while selecting target ops
  -> MIR carries target machine-ish blocks and lowers target pseudos
  -> encoder/internal assembler emits bytes, labels, fixups, relocations
  -> ELF object
```

The architecture is correct only if each layer receives information from the
layer above it rather than rediscovering that information from textual asm or
object-emission side effects.

## Execution Rules

- Start with boundary evidence before editing backend implementation.
- Keep each implementation packet narrow and prove it with representative RV64
  object link-and-qemu execution.
- If a representative case also hits globals/strings/data-section blockers,
  classify that separately and do not hide it inside the multi-block repair.
- Reuse existing RV64 assembler/encoder/fixup code only for low-level encoding
  responsibilities, not for CFG recovery.
- Keep `.s`, inline asm, `c4c-as`, and `c4c-objdump` behavior green while
  changing object emission.

## Step 1: Audit the Multi-Block Rejection Boundary

Goal: identify where representative multi-block cases are rejected before any
repair is attempted.

Concrete actions:

- Run a narrow gcc torture backend scan for the representative cases with
  `CASE_TIMEOUT_SEC=20`.
- For each case, record whether rejection occurs at:
  - BIR/prepared-BIR shape,
  - RV64 target block preservation,
  - MIR/pseudo lowering,
  - object emission single-block gate,
  - or an unrelated globals/strings/ABI blocker.
- Inspect the prepared RV64 object route enough to name the exact gate,
  function, and diagnostic text.
- Compare against RV64 `.s` emission if useful to determine whether target
  block selection already exists before object emission rejects the case.
- Do not edit backend implementation in this step.

Completion check:

- `todo.md` contains a concise boundary table for all representative cases.
- The next implementation step can name the first real layer to repair.
- No implementation files were touched.

## Step 2: Define the Object-Emission Handoff Contract

Goal: make explicit what RV64 object emission consumes after BIR-owned CFG has
already been target-lowered.

Concrete actions:

- Identify the current prepared-module/function data structure handed to
  `object_emission.cpp`.
- Define the required target-block/instruction/label/fixup shape without
  encoding high-level CFG semantics in object emission.
- Decide where shared RV64 instruction encoding should be called from the
  object route.
- Add focused contract tests or diagnostics only if needed to lock the handoff.

Completion check:

- The handoff contract is documented in code/tests or a durable runbook note.
- The contract does not require assembler text to be the semantic IR.
- Existing tests still build.

## Step 3: Replace the Blanket Single-Block Gate

Goal: allow valid prepared multi-block target streams through object emission
without bypassing semantic lowering.

Concrete actions:

- Replace the single-block rejection with checks on the target-block stream
  shape required by Step 2.
- Preserve fail-closed diagnostics for cases still missing data sections,
  globals, strings, call ABI, or unsupported instruction forms.
- Keep labels and branches explicit as target references, not recovered CFG.

Completion check:

- Representative cases no longer fail only because a function has multiple
  blocks.
- Unsupported cases report the more precise next blocker.
- No testcase-name special casing was introduced.

## Step 4: Share Low-Level RV64 Encoding and Fixups

Goal: keep object emission and textual assembler from growing divergent RV64
encoding/fixup implementations.

Concrete actions:

- Route object emission through existing RV64 encoder/line assembler helpers
  where the abstraction matches target instructions and labels.
- Add missing helper boundaries only for bytes, immediates, labels, fixups,
  relocations, and ELF section data.
- Avoid moving CFG or liveness knowledge into assembler code.

Completion check:

- Branch/call labels and immediate fixups are handled by low-level machinery.
- `.s`/`c4c-as` behavior remains compatible with object emission encoding.
- Curated RV64 assembler/object tests remain green.

## Step 5: Prove Representative Runtime Correctness

Goal: prove the repaired route links and runs representative RV64 cases.

Concrete actions:

- Run the narrow allowlist through
  `scripts/check_progress_rv64_gcc_c_torture_backend.sh`.
- Run existing RV64 object/runtime, assembler, objdump, and inline asm tests
  that cover touched surfaces.
- Compare failure buckets before and after the repair.

Completion check:

- Representative cases either pass link-and-qemu execution or fail with a
  precise out-of-scope blocker owned by ideas 357/358.
- Existing touched-surface tests pass.

## Step 6: Milestone Scan and Handoff

Goal: quantify the effect on the gcc torture backend scan and identify the next
child idea to execute.

Concrete actions:

- Run the full opt-in gcc torture backend scan with `CASE_TIMEOUT_SEC=20`.
- Compare the `multi_block_control_flow` or successor structured bucket against
  the previous 540-case baseline.
- Record remaining blockers in `todo.md` and recommend whether to continue
  356, switch to 357, or split a new idea.

Completion check:

- The full scan shows a real bucket movement or produces precise diagnostics
  explaining why no movement is possible yet.
- The handoff does not claim capability progress from classification-only work.

## Validation Ladder

- Lifecycle/audit-only packets: no build required unless code is touched.
- Code packets: build the affected backend targets first.
- Narrow proof:

```sh
CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-multiblock-allowlist.txt \
  scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

- Milestone proof:

```sh
CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

## Reviewer Reject Signals

- Reject testcase-name shortcuts or filename-specific lowering.
- Reject expectation downgrades, unsupported-test expansion, or weaker runtime
  comparison as proof of progress.
- Reject any MIR, assembler, or object-emission code that reconstructs CFG
  semantics that should be owned by BIR.
- Reject direct object encoder growth that duplicates RV64 encoding/fixup logic
  while leaving the architecture split unresolved.
- Reject silent dropping of blocks, calls, declarations, branch edges, or inline
  asm operand relationships.
