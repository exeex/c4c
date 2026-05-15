# AArch64 Memory Load Store Machine Nodes

Status: Closed
Created: 2026-05-14

Parent Context: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md

## Problem

The AArch64 backend has structured memory operand records and prepared memory
access facts, but block dispatch does not yet lower BIR load/store
instructions into selected memory machine nodes. `MemoryInstructionRecord`
can carry prepared frame-slot and pointer-value memory operands, and the
terminal printer has a narrow store path, but load nodes still lack a
structured destination register in the printable subset and memory
instructions are not reached by current dispatch.

This is a current AArch64 lowering gap, not a reason to revive the archived
`memory.cpp` scratch-register surface.

## Scope

- Lower prepared BIR local/global load and store instructions into AArch64
  `MemoryInstructionRecord` machine nodes.
- Consume `PreparedMemoryAccess` addressing facts for frame-slot and
  pointer-value base-plus-offset accesses.
- Add the missing structured load destination register path and complete the
  printable load/store subset only when register and address facts are
  explicit.
- Preserve global address materialization, dynamic-stack/frame setup, memcpy,
  memset, and F128 full-width transport for separate focused routes.

## Non-Goals

- Do not reconstruct the archived `memory.cpp` accumulator and scratch
  register convention.
- Do not infer prepared address facts inside AArch64 codegen.
- Do not conflate global symbol memory access with global address
  materialization.
- Do not add named-case load/store shortcuts or weaken backend expectations.

## Proof Direction

- A frame-slot load emits a selected load node with a structured destination
  register and prepared memory-read side effect.
- A frame-slot or pointer-value store emits a selected store node from
  prepared address and stored-value facts.
- Unsupported symbol/string or unprepared bases remain deferred with explicit
  diagnostics rather than silently printing invalid assembly.

## Closure Notes

Closed: 2026-05-15

The active runbook completed the intended semantic subset:

- frame-slot loads lower through prepared memory facts, selected structured
  destination-register authority, and terminal `ldr` output
- frame-slot stores lower through prepared memory facts, selected structured
  source-register operands, and terminal `str` output
- pointer-value stores lower through prepared pointer base identity, selected
  structured pointer base/source registers, and terminal `str` output
- unsupported and unprepared memory bases remain fail-closed through explicit
  diagnostics
- global address materialization remains separate from global memory access

Pointer-value loads and selected global/symbol memory access remain outside
this closed route and should be handled by a separate source idea if they
become required.

Close proof used the accepted full-suite regression artifact in
`test_before.log`: 3167/3167 tests passed.
