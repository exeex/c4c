# RV64 Translated Codegen Integration

Status: Open
Last Updated: 2026-04-13

## Why This Idea

The repository already contains a large translated RV64 backend surface under
`src/backend/riscv/`, including assembler, codegen, linker, and target-entry
files. The main problem is no longer translation coverage. The problem is that
translated RV64 inventory still has to be turned into a truthful backend path.

The durable framing is:

- treat translated RV64 source as real assets already present in the tree
- make bounded owner surfaces compile, connect, and prove real behavior
- avoid drifting into broad runtime, CRT, shared-library, or toolchain bring-up
  before the underlying translated backend seams are truthful

## Goal

Bring translated RV64 codegen forward in bounded, truthful ownership slices
until the translated backend path is meaningfully real.

Concretely:

- connect translated RV64 codegen, assembler/object emission, and linker seams
- keep proving slices through real emitted-object or backend behavior, not only
  through synthetic handwritten fixtures
- move one family at a time from translated inventory to truthful active owner
- preserve clear boundaries between translated-backend integration and broader
  runtime/toolchain bring-up

## Non-Goals

- do not treat this as greenfield RV64 backend development
- do not widen immediately into CRT/library integration, shared-library
  linking, runtime bring-up, or qemu execution
- do not silently absorb unrelated x86 work or shared generic backend cleanup
- do not mutate the idea every time one packet lands

## Durable Constraints

- stay slice-bounded: one family, one seam, one proof story at a time
- prefer real translated-owner progress over handcrafted fixture accumulation
- keep `todo.md` and `plan.md` as the place for packet history; this idea
  should stay durable
- when one family is exhausted, require a fresh route decision instead of
  guessing the next lane by momentum

## Main Family Inventory

The broad RV64 translated inventory still spans families such as:

- assembler and object emission
- linker and relocation handling
- calls and calling-convention lowering
- variadic support
- globals and TLS handling
- memory, typed-indirect, and broader GEP work
- integer-owner and comparison-owner work
- F128 / soft-float support
- atomics and other parked owner files

Future activations should consume bounded slices from these families rather
than rewriting this idea into a packet log.

## Current Durable State

- the active lifecycle route is still inside translated RV64 backend
  integration, not broad runtime or toolchain bring-up
- recent execution has already made multiple bounded translated seams real, so
  the next work should continue from truthful landed surfaces rather than
  retelling the full commit history
- the current active theme is binary-utils-backed translated codegen progress:
  proving codegen-produced RV64 objects through the existing assembler/object
  and static-link seams before widening into broader families
- broader outgoing-call, runtime, dependency-loading, shared-library, and
  execution-environment work remain intentionally parked unless a bounded route
  proves they are now the smallest truthful next blocker

## Reactivation Notes

When this idea becomes active or is rewritten into a fresh plan again:

- start from the currently truthful translated RV64 family inventory
- choose one bounded owner seam or family checkpoint
- make the minimum shared header/build/proof surface needed for that seam real
- prefer emitted-object and backend-path proof over synthetic fixture growth
- if the next required step becomes broad runtime/toolchain work, stop and open
  or activate a separate idea instead of stretching this one
