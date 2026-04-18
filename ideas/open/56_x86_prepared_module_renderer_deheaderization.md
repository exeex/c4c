# X86 Prepared-Module Renderer De-Headerization

Status: Open
Created: 2026-04-18

## Why This Idea Exists

`src/backend/mir/x86/codegen/x86_codegen.hpp` is currently carrying a large
inline implementation blob instead of only shared API and type declarations.
The file is roughly `3760` lines, and the full prepared-module renderer starts
in the header and continues to the end of the file.

That shape is no longer defensible as a shared contract surface for sibling x86
translation units.

## Source Evidence

Reviewer report:

- [review/x86_codegen_header_split_review.md](/workspaces/c4c/review/x86_codegen_header_split_review.md)

Key conclusions from that review:

- `x86_codegen.hpp` is carrying target validation, lane selection, local-slot
  layout, control-flow pattern recognition, and final asm rendering logic
- the right first move is to preserve the public handoff contract in the
  header while moving the main prepared-module renderer flow into `.cpp`
  ownership
- this should be treated as its own route change or open idea, not hidden
  inside the active x86 local-memory capability packet

## Goal

Reduce `x86_codegen.hpp` back to a shared contract header by moving renderer
implementation into `.cpp` translation units without changing behavior.

## Core Rules

- keep the public handoff contract and shared types available to sibling x86
  translation units
- `src/backend/mir/x86/codegen/x86_codegen.hpp` should keep only shared
  contract, shared types, and public declarations
- the main `emit_prepared_module(...)` orchestration flow should live in
  compiled `.cpp` ownership, ideally under existing x86 codegen owners and, if
  those owners are not yet coherent build surfaces, under a focused support
  file such as `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- the first implementation packet may both remove implementation from the
  header and redistribute ownership into existing
  `src/backend/mir/x86/codegen/*.cpp` files
- split by ownership under the prepared-module renderer, not by testcase name
  or arbitrary line counts
- keep AArch64 out of scope

## First Packet

The first packet is a behavior-preserving de-headerization plus ownership
redistribution step. Its default target shape is the repo's existing one-header
and multiple-source design, not a temporary monolithic renderer `.cpp`.

The first implementation should:

- keep the public API shape unchanged
- keep behavior and current lane selection unchanged
- move the main `emit_prepared_module(...)` orchestration flow into compiled
  `.cpp` ownership, preferring existing x86 codegen owners when they are
  coherent build surfaces
- directly redistribute semantic ownership into existing
  `src/backend/mir/x86/codegen/*.cpp` units where those owners are already
  natural
- introduce only small support helpers if no existing owner is suitable

The header should retain declarations for:

- `emit_prepared_module(...)`
- `emit_module(const bir::Module&)`
- `emit_module(const lir::LirModule&)`
- `assemble_module(...)`

## Ownership Seams

Favor these semantic seams when moving implementation out of the header:

- memory and local-slot ownership
- calls and multi-function handoff ownership
- comparison and control-flow ownership
- globals and data emission ownership
- small shared support helpers only when needed

These seams should normally map into existing
`src/backend/mir/x86/codegen/*.cpp` files such as `emit.cpp`, `memory.cpp`,
`calls.cpp`, `comparison.cpp`, and `globals.cpp`, with new helper `.cpp` files
such as `prepared_module_emit.cpp` added only if the existing ownership
boundaries cannot absorb the code cleanly.

## Primary Targets

- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- `src/backend/mir/x86/codegen/emit.cpp`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- existing `src/backend/mir/x86/codegen/*.cpp` files that naturally own the
  extracted renderer pieces
- small new support `.cpp` files under `src/backend/mir/x86/codegen/` only if
  existing owners are not coherent

## Acceptance Criteria

- [ ] `x86_codegen.hpp` stops owning the full prepared-module renderer body
- [ ] `x86_codegen.hpp` is reduced to shared contract, shared types, and public
      declarations
- [ ] a compiled `.cpp` owner, currently
      `prepared_module_emit.cpp`, owns the main
      `emit_prepared_module(...)` orchestration flow
- [ ] the first packet preserves behavior and public API shape while allowing
      direct redistribution into existing x86 codegen `.cpp` files
- [ ] narrow x86 handoff proof passes after the move
- [ ] any deeper sub-splits are justified by ownership, not convenience
- [ ] no new backend capability-family claim is mixed into this refactor

## Validation

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary(_test)?|backend_lir_to_bir_notes)$'`
- any additional narrow x86 prepared-module proofs current at activation time

## Non-Goals

- landing new x86 backend capability-family support as part of this refactor
- splitting `src/backend/bir/lir_to_bir_memory.cpp`
- AArch64 codegen cleanup
