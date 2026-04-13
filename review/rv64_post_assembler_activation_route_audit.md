# RV64 Post-Assembler-Activation Route Audit

Date: 2026-04-13
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `f2379d9d`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to assembler activation family`)
- why this is the right checkpoint:
  `scripts/plan_change_gap.sh` reports `f2379d9d` as the latest canonical
  lifecycle checkpoint, and that commit is the rewrite that selected the
  bounded RV64 assembler / binary-utils activation route plus the first
  activation-sized packet now under review
- commits since base: `1`
  (`5c078b98 rv64: activate assembler object handoff seam`)

## Scope Reviewed

- `plan.md`
- `todo.md`
- `review/rv64_post_native_peephole_broader_route_audit.md`
- `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`
- `CMakeLists.txt`
- `src/backend/backend.cpp`
- `src/backend/riscv/assembler/mod.cpp`
- `src/backend/riscv/assembler/mod.hpp`
- `src/backend/riscv/assembler/parser.cpp`
- `src/backend/riscv/assembler/elf_writer.cpp`
- `src/backend/riscv/codegen/emit.hpp`
- `src/backend/riscv/codegen/emit.cpp`
- `tests/backend/backend_shared_util_tests.cpp`

## Findings

- Medium: commit `5c078b98` truthfully completes the bounded RV64 assembler
  activation packet selected by the prior broader-route audit. The shared
  backend handoff now routes `Target::Riscv64` through a named RV64 assembler
  request/result seam instead of hardcoding `object_emitted = false`, the
  minimum RV64 assembler sources are wired into the active build graph, and
  shared-util proof now checks RV64 wrapper parity plus parsing of the emitted
  RV64 object:
  [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp:197),
  [src/backend/riscv/codegen/emit.cpp](/workspaces/c4c/src/backend/riscv/codegen/emit.cpp:125),
  [CMakeLists.txt](/workspaces/c4c/CMakeLists.txt:220),
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:9096).
- Medium: the newly-landed RV64 assembler seam is still an activation scaffold,
  not yet a truthful direct assembler packet. `assemble(...)` and the current
  ELF writer only accept the exact five-line activation text and then synthesize
  fixed instruction bytes and ELF structure directly, while the newly-compiled
  translated parser inventory is not yet consumed by the public assembly path:
  [src/backend/riscv/assembler/mod.cpp](/workspaces/c4c/src/backend/riscv/assembler/mod.cpp:58),
  [src/backend/riscv/assembler/mod.cpp](/workspaces/c4c/src/backend/riscv/assembler/mod.cpp:78),
  [src/backend/riscv/assembler/elf_writer.cpp](/workspaces/c4c/src/backend/riscv/assembler/elf_writer.cpp:259),
  [src/backend/riscv/assembler/elf_writer.cpp](/workspaces/c4c/src/backend/riscv/assembler/elf_writer.cpp:278),
  [src/backend/riscv/assembler/elf_writer.cpp](/workspaces/c4c/src/backend/riscv/assembler/elf_writer.cpp:636).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state records
commit `5c078b98` as the completed RV64 assembler / binary-utils activation
packet and then narrows the active route to the first direct assembler packet:

- make the current RV64 activation slice truthful through the newly-compiled
  assembler surface instead of exact-text matching and handwritten instruction
  bytes / ELF layout
- stay bounded to the current emitted activation slice shape already proven by
  `assemble_target_lir_module(...)` and the new RV64 wrapper parity test
- keep broader assembler feature expansion, linker activation / executable
  finalization, broader `emit.cpp` support expansion, and the other parked
  RV64 families inactive

The narrowest honest follow-on is not linker work yet. It is the first direct
assembler packet inside the newly-activated lane: consume the current bounded
RV64 emitted text through the parser/object path for the same minimal
`.text` / `.globl main` / `main:` / `addi a0, zero, 5` / `ret` slice before
widening into more instructions, relocations, or linker-facing work.

## Rationale

The activation goal in the current plan was to make the RV64 assembler seam
real in the top-level backend path and build graph. That has now happened:
RV64 object handoff exists at the shared backend surface, the build graph
includes the minimum assembler files, and the proof demonstrates wrapper
parity plus a parseable emitted ELF object.

That means the activation packet itself is done. The remaining gap is no longer
"wire the seam" but "make the seam truthful." The implementation still matches
exact activation text and injects fixed bytes and ELF structure directly, so
the next honest slice should stay inside the assembler lane and convert this
activation scaffold into the first real parser-backed / assembler-backed packet
for the same already-proven minimal emitted slice.
