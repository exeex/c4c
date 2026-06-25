# RV64 Object Route via Prepared Text and Internal Assembler

Status: Open
Type: Architecture repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Consolidate RV64 `--codegen obj` around the prepared RV64 assembly emitter and
the reusable internal assembler/object writer so multi-block prepared BIR,
branches, calls, labels, and scalar instruction coverage do not have to be
duplicated in a narrow direct object encoder.

## Why This Exists

The prepared-shape classification found `540` primary failures in
`multi_block_control_flow`, plus smaller call/local-memory/declaration buckets.
`object_emission.cpp` currently imposes a single-block function gate and a
small direct fragment encoder, while `prepared_module_emit.cpp` already owns a
broader prepared-asm lowering surface. Keeping both paths independent makes
future repairs architecture-risky and testcase-overfit-prone.

## In Scope

- Decide the canonical RV64 object route architecture for prepared BIR.
- Prefer a route shaped like:
  `PreparedBirModule -> RV64 prepared asm text -> internal RV64 assembler -> ObjectModule/ELF`.
- Reuse the same single-line parser/assembler logic used by c4c-as and inline
  asm where possible.
- Support labels, branch fixups, same-module calls, external call relocations,
  prologue/epilogue text, and ordinary scalar instructions emitted by the
  prepared RV64 asm path.
- Preserve direct `.insn.d` and `.insn r` inline asm object emission behavior.

## Out of Scope

- Adding full RV64 data-section support; that belongs to
  `ideas/open/357_rv64_object_route_data_sections_globals_strings.md`.
- Implementing every missing scalar opcode by testcase name.
- Changing frontend or BIR semantics.
- Requiring the heavy gcc torture scan in default full CTest.

## Representative Cases

- `src/20000113-1.c`: multi-block CFG with bitfield-style scalar ops and
  external calls.
- `src/20000205-1.c`: multi-block scalar control flow.
- `src/20010119-1.c`: general call lowering shape.
- `src/20030216-1.c`: external declaration/control-flow entry.

## Suspected Stage

RV64 prepared object emission architecture, internal assembler integration,
and object relocation construction.

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

- The object route no longer rejects representative single-module multi-block
  functions solely because `function->blocks.size() != 1`.
- Assembler-backed object emission handles labels and branch/call relocations
  produced by the prepared RV64 asm path.
- Existing `c4c-as`, `c4c-objdump`, inline asm, and curated RV64 object tests
  remain green.
- The full gcc torture backend scan shows a real reduction in
  `multi_block_control_flow` or its successor structured diagnostic bucket.

## Reviewer Reject Signals

- Reject if the fix special-cases the listed torture filenames.
- Reject if it extends the direct encoder with unrelated one-off instruction
  snippets while leaving the duplicated architecture unresolved.
- Reject if labels/relocations work only for one known CFG shape.
- Reject if the route silently drops basic blocks, calls, declarations, or
  branch edges to make object emission succeed.
- Reject if proof relies on expectation weakening instead of executable RV64
  link-and-qemu correctness.

