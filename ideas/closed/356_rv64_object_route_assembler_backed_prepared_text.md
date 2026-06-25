# RV64 Object Route With BIR-Owned CFG and Shared Encoding

Status: Closed
Type: Architecture repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Repair the RV64 `--codegen obj` route for multi-block prepared functions
without moving CFG semantics into MIR, the assembler, or the object writer.

The intended architecture is:

```text
C/C++ frontend
  -> HIR
  -> BIR owns CFG, semantic control flow, and data-flow edges
  -> RV64 target route preserves BIR blocks/edges while selecting target ops
  -> MIR only carries target machine-ish blocks and lowers target pseudos
     such as spill, reload, frame, and call-sequence details
  -> encoder/internal assembler emits bytes, labels, fixups, relocations
  -> ELF object
```

The object route must not infer `if`, `loop`, `switch`, block reachability, or
data-flow semantics from assembly text. It should consume already-lowered
target blocks/instructions and share instruction encoding, label/fixup, and
relocation machinery with the textual assembler where that avoids duplicating
the direct object encoder.

## Why This Exists

The prepared-shape classification found `540` primary failures in
`multi_block_control_flow`, plus smaller call/local-memory/declaration buckets.
`object_emission.cpp` currently imposes a single-block function gate and a
small direct fragment encoder.

The risk is not that the assembler needs to understand multi-block control
flow. The risk is that RV64 object emission may grow a second target encoding
and label/fixup path beside the `.s`/`c4c-as` route while still rejecting BIR
CFG shapes. That would duplicate machinery at the wrong layer and hide whether
the real blocker is:

- BIR/prepared-BIR failed to model the CFG,
- the RV64 target route failed to preserve BIR blocks/edges into target blocks,
- MIR/pseudo lowering failed after target blocks already exist, or
- object emission rejected a valid target-block stream because the direct
  encoder only accepts single-block fragments.

This idea owns that boundary decision and the first repair. It should make the
layering explicit before broad multi-block execution fixes land.

## In Scope

- Audit the current RV64 prepared object route and identify exactly where
  multi-block functions are rejected:
  BIR/prepared-BIR shape, target block preservation, MIR/pseudo lowering, or
  object emission.
- Define the canonical data structure handed to RV64 object emission. It
  should represent target machine blocks/instructions with explicit label
  references, not high-level CFG semantics and not raw source-level assembly
  strings as the only truth.
- Remove or replace the blanket single-block object-emission gate when the
  input already contains valid BIR-owned CFG lowered into target blocks.
- Share RV64 instruction encoding and low-level label/fixup/relocation
  machinery with `c4c-as` / the internal assembler where possible.
- Keep assembler responsibilities limited to machine-code encoding, immediate
  range checks, label offset fixups, and relocation records.
- Keep MIR responsibilities limited to target representation and pseudo
  lowering such as spill/reload/frame/call sequence lowering; MIR must not
  invent or recover CFG semantics.
- Preserve direct `.insn.d` and `.insn r` inline asm object emission behavior,
  with inline asm register relationships still visible to BIR/MIR/RA before
  object emission.

## Out of Scope

- Adding full RV64 data-section support; that belongs to
  `ideas/open/357_rv64_object_route_data_sections_globals_strings.md`.
- Implementing every missing scalar opcode by testcase name.
- Moving CFG ownership out of BIR.
- Making the assembler parse or recover semantic control flow.
- Treating textual assembly as a semantic IR.
- Requiring the heavy gcc torture scan in default full CTest.
- Broad ABI/width cleanup beyond the blockers needed to prove the
  multi-block object-route architecture.

## Representative Cases

- `src/20000113-1.c`: multi-block CFG with bitfield-style scalar ops and
  external calls.
- `src/20000205-1.c`: multi-block scalar control flow.
- `src/20010119-1.c`: general call lowering shape.
- `src/20030216-1.c`: external declaration/control-flow entry.

## Suspected Stage

Boundary between prepared BIR CFG, RV64 target block lowering, MIR pseudo
lowering, and RV64 object emission. The object writer/assembler layer should
only own encoding, labels, fixups, relocations, and ELF construction.

## Proof Command

Narrow proof:

```sh
CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-multiblock-allowlist.txt \
  scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

where the allowlist contains representative cases such as:

```text
src/20000113-1.c
src/20000205-1.c
src/20010119-1.c
src/20030216-1.c
```

Milestone proof:

```sh
CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

## Acceptance Criteria

- The implementation or analysis documents which layer rejected each
  representative multi-block case before repair.
- BIR remains the owner of CFG semantics; no assembler/object-writer code
  attempts to infer high-level control flow from text.
- The object route no longer rejects representative single-module multi-block
  functions solely because the prepared function has more than one block,
  when those blocks have already been lowered into valid target blocks.
- RV64 object emission can encode labels and branch/call relocations from the
  target block stream without duplicating CFG lowering.
- Existing `c4c-as`, `c4c-objdump`, inline asm, and curated RV64 object tests
  remain green.
- The full gcc torture backend scan shows a real reduction in
  `multi_block_control_flow` or its successor structured diagnostic bucket.

## Reviewer Reject Signals

- Reject if the fix special-cases the listed torture filenames.
- Reject if it extends the direct encoder with unrelated one-off instruction
  snippets while leaving the duplicated architecture unresolved.
- Reject if MIR, assembler, or object emission starts reconstructing CFG
  semantics that should have come from BIR.
- Reject if the assembler is asked to understand loops, switches, if/else
  structure, liveness, or inline-asm operand relationships.
- Reject if labels/relocations work only for one known CFG shape.
- Reject if the route silently drops basic blocks, calls, declarations, or
  branch edges to make object emission succeed.
- Reject if proof relies on expectation weakening instead of executable RV64
  link-and-qemu correctness.

## Closure Notes

Closed after Step 5 representative proof reached 4/4 for
`src/20000113-1.c`, `src/20000205-1.c`, `src/20010119-1.c`, and
`src/20030216-1.c`. The accepted route kept BIR/prepared ownership of CFG and
data-flow semantics, used the shared prepared traversal stream, and retired the
duplicate staged RV64 assembler minimal JAL object branch.

The Step 4 ownership audit classified the RV64 object fragment/fixup/module
builder as the low-level object concern, `c4c-as.cpp` local label resolution as
textual assembler-local, and the old `assembler/elf_writer.cpp` minimal JAL
relocation slice as duplicate production machinery to remove. The duplicate
path was removed before closure.

Milestone RV64 gcc torture backend scan result recorded by the supervisor:
`total=1467 passed=70 failed=1397`. Current case logs show the successor
generic `prepared module shape` bucket reduced from the parent classification's
`1012` failures to `942` remaining failures. Remaining globals, strings, ABI,
width, and other prepared-shape families stay with their separate open ideas.

Close gate: full CTest regression guard passed in non-decreasing mode using
canonical `test_before.log` and `test_after.log`; both logs reported
`passed=3352 failed=0 total=3352`.
