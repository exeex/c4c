# Prealloc Agent Index Header Hierarchy And Rust Reference Removal

Status: Active
Source Idea: ideas/open/prealloc-agent-index-header-hierarchy-and-rust-reference-removal.md

## Purpose

Make `src/backend/prealloc` easier for agents to navigate by using path depth
as the visibility signal and by removing prealloc-local Rust reference files
that are no longer active implementation.

## Goal

Leave prealloc with explicit exported top-level headers, one private
stack-layout index under `src/backend/prealloc/stack_layout/`, no local
prealloc `.rs` files, and unchanged backend behavior.

## Core Rule

Treat directories as semantic boundaries and headers as index entries for those
boundaries:

- `src/backend/prealloc/*.hpp` is exported prealloc surface unless explicitly
  proven implementation-only.
- `src/backend/prealloc/stack_layout/*.hpp` is private stack-layout
  implementation surface unless promoted by a real cross-boundary need.
- Do not introduce one header per `.cpp`.

## Read First

- `ideas/open/prealloc-agent-index-header-hierarchy-and-rust-reference-removal.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepared_printer.hpp`
- `src/backend/prealloc/target_register_profile.hpp`
- `src/backend/prealloc/stack_layout/support.hpp`
- `src/backend/prealloc/README.md`

## Scope

- Classify the top-level prealloc headers as exported surfaces or merge
  candidates.
- Rename the stack-layout private index from `support.hpp` to a clearer
  directory-level header such as `stack_layout.hpp`.
- Consider moving the stack-layout coordinator from
  `src/backend/prealloc/stack_layout.cpp` to
  `src/backend/prealloc/stack_layout/coordinator.cpp` if the include and build
  shape supports it cleanly.
- Delete prealloc-local reference-only Rust files under
  `src/backend/prealloc`.
- Update prealloc-local documentation that still points agents at those Rust
  files as active guidance.

## Non-Goals

- Do not change prealloc semantics, stack-layout behavior, register allocation
  behavior, printer output, or testcase expectations.
- Do not delete or reorganize `ref/claudes-c-compiler/**`.
- Do not create a traditional separated `include/` tree.
- Do not split every implementation file into a matching header.
- Do not treat expectation rewrites as progress for this structural cleanup.

## Working Model

- Top-level prealloc headers are visible package-boundary APIs.
- Nested stack-layout headers are private implementation indexes.
- Thin headers should either be justified as exported surfaces or merged into
  the appropriate boundary index.
- The local Rust files were porting references; they should not remain beside
  the active C++ implementation once the C++ code is the source of truth.

## Execution Rules

- Keep each code-changing step behavior-preserving.
- Prefer include/user inspection before moving or merging headers.
- Update CMake only if the existing source discovery does not pick up moved
  files.
- Use `todo.md` for packet progress and proof notes; do not rewrite this plan
  for routine packet completion.
- If deleting a Rust reference uncovers a semantic discrepancy, record it as a
  separate idea unless a documentation-only clarification is enough.
- Build after every structural code slice. Escalate to relevant backend or full
  CTest coverage before closure.

## Ordered Steps

### Step 1: Decide Exported Top-Level Prealloc Surfaces

Goal: Classify top-level headers before changing the hierarchy.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepared_printer.hpp`
- `src/backend/prealloc/target_register_profile.hpp`

Actions:

- Inspect include users for each top-level prealloc header.
- Keep `prealloc.hpp` as the main exported prealloc model and phase API.
- Decide whether `target_register_profile.hpp` is still needed outside the
  prealloc implementation.
- Decide whether `prepared_printer.hpp` should stay as an exported printer
  surface or merge into `prealloc.hpp`.
- Record classifications and any merge decision in `todo.md`.

Completion check:

- Each top-level header has an explicit classification: exported, merge
  candidate, or private-move candidate.
- No implementation files are changed in this step unless the supervisor
  explicitly delegates a combined inspection-and-edit packet.

### Step 2: Rename the Stack-Layout Private Index

Goal: Replace the generic nested stack-layout header name with a semantic
directory index.

Primary target:

- `src/backend/prealloc/stack_layout/support.hpp`

Actions:

- Rename `support.hpp` to
  `src/backend/prealloc/stack_layout/stack_layout.hpp`.
- Update stack-layout implementation includes and any legitimate prealloc
  callers.
- Keep implementation-only helpers behind the nested stack-layout index.
- Do not create per-file stack-layout headers.

Completion check:

- No source file includes
  `src/backend/prealloc/stack_layout/support.hpp` or `"support.hpp"`.
- The renamed nested header is the single private stack-layout index.
- `cmake --build --preset default --target c4c_backend` succeeds, or the
  supervisor delegates an equivalent build proof.

### Step 3: Move the Stack-Layout Coordinator If Clean

Goal: Align the coordinator implementation with the stack-layout semantic
directory when the move is straightforward.

Primary target:

- `src/backend/prealloc/stack_layout.cpp`

Actions:

- Inspect how the coordinator is built and included.
- Move `src/backend/prealloc/stack_layout.cpp` to
  `src/backend/prealloc/stack_layout/coordinator.cpp` if the build and local
  naming make the move mechanical.
- Keep public orchestration declarations in `prealloc.hpp`.
- If the move is not cleanly mechanical, record the reason in `todo.md` and
  leave it for a separate idea rather than widening this plan.

Completion check:

- Either the coordinator lives under `src/backend/prealloc/stack_layout/` and
  the backend builds, or `todo.md` records why the move was intentionally
  skipped.
- No prealloc behavior or public API semantics changed.

### Step 4: Remove Prealloc-Local Rust Reference Files

Goal: Delete stale local Rust reference files from the active prealloc source
tree.

Primary targets:

- `src/backend/prealloc/liveness.rs`
- `src/backend/prealloc/regalloc.rs`
- `src/backend/prealloc/stack_layout/*.rs`

Actions:

- Delete the prealloc-local `.rs` files.
- Confirm they are not build inputs.
- Update `src/backend/prealloc/README.md` and nearby comparison notes that
  tell agents to consult those local `.rs` files as active guidance.
- Do not delete `ref/claudes-c-compiler/**`.

Completion check:

- `find src/backend/prealloc -name '*.rs'` reports no files.
- Prealloc-local docs no longer point to deleted local Rust files as active
  implementation references.
- `cmake --build --preset default --target c4c_backend` succeeds, or the
  supervisor delegates an equivalent build proof.

### Step 5: Final Structural Validation and Cleanup

Goal: Prove the hierarchy cleanup is coherent and behavior-preserving.

Actions:

- Re-scan `src/backend/prealloc` headers for the intended visibility signal.
- Confirm top-level headers are either exported surfaces or explicitly
  justified thin surfaces.
- Confirm stack-layout has one nested private index header.
- Confirm no one-header-per-`.cpp` pattern was introduced.
- Run the supervisor-selected relevant backend/prealloc test subset.
- Escalate to full CTest or regression guard if the supervisor treats this as a
  closure milestone.

Completion check:

- Source-idea acceptance criteria are satisfied.
- Build and relevant backend/prealloc tests pass.
- No testcase expectations were weakened.
- `todo.md` contains enough proof for the supervisor to request lifecycle
  closure.
