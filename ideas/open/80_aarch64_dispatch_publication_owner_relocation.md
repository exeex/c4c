# AArch64 Dispatch Publication Owner Relocation

## Goal

Classify and relocate remaining target-local publication helpers from
`dispatch_publication.cpp`, `dispatch_value_materialization.cpp`, and
`dispatch_producers.cpp` into the narrower AArch64 owner files that actually
consume them, while preserving shared prepared-publication authority.

## Why This Exists

The dispatch-family cleanup and prepared-authority routes moved publication
selection toward shared prepared facts, but AArch64 still carries synthetic
dispatch publication files that do not match the reference ARM owner layout.
Many helpers now look like owner-local emission support for memory, globals,
scalar, compare, fixed-formal store publication, or current-block join
publication.

## Owned Files

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- Narrow destination owners only as justified by the classification:
  `memory.cpp`, `globals.cpp`, `alu.cpp`, `comparison.cpp`, `calls.cpp`, or
  `dispatch.cpp`.

## In Scope

- Build a helper-by-helper classification table before moving code.
- Fold pure target-local helpers into the owner that owns the emitted record or
  instruction family.
- Keep shared prepared publication facts in shared prealloc/BIR surfaces.
- Keep dispatch-route orchestration in `dispatch.cpp` when the helper is truly
  cross-owner dispatch glue.
- Preserve diagnostics and machine-record behavior.

## Out Of Scope

- Inventing new shared prepared-publication authority unless a concrete missing
  fact is proven.
- Mixing edge-copy relocation into this route.
- Rewriting instruction records or machine printer spelling.
- Shrinking line count by weakening tests, deleting diagnostics, or hiding
  behavior behind named-case shortcuts.

## Proof Expectations

- Focused build proof after each relocation slice.
- Backend CTest subset covering AArch64 dispatch publication, same-block
  publication, fused-compare publication, fixed-formal store publication, and
  current-block join publication.
- Regression guard before closure if multiple owners are touched.

## Reviewer Reject Signals

- A helper is moved but still re-derives prepared publication authority locally.
- Broad dispatch rewrites are mixed with unrelated call, memory, or printer
  cleanup.
- AArch64 target-local register spelling or record construction is moved into
  shared prealloc.
- The route claims success only by reducing file count without preserving owner
  clarity.

