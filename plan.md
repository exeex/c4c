# BIR Agent Index Header Hierarchy Runbook

Status: Active
Source Idea: ideas/open/bir-agent-index-header-hierarchy.md

## Purpose

Optimize `src/backend/bir` for agent-oriented editing by treating headers as
directory-level index surfaces instead of one-to-one `.cpp` partners.

## Goal

Make the BIR header hierarchy reflect semantic directory boundaries while
preserving source-level behavior and backend test expectations.

## Core Rule

`directory = semantic boundary`; each `.hpp` should be an index for that
boundary, not a mandatory partner for one implementation file.

## Read First

- `ideas/open/bir-agent-index-header-hierarchy.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_to_bir/*.cpp`
- `CMakeLists.txt` and nested CMake files that enumerate BIR sources

## Current Scope

- Keep the BIR public data model centralized in `src/backend/bir/bir.hpp`.
- Shrink `src/backend/bir/lir_to_bir.hpp` to the exported LIR-to-BIR entry API.
- Create `src/backend/bir/lir_to_bir/lowering.hpp` as the private directory
  index for lowering implementation declarations.
- Move memory lowering files into `src/backend/bir/lir_to_bir/memory/` only
  after the public/private header boundary is clean.
- Add `memory/memory.hpp` only if memory-specific shared declarations need a
  subdomain index after the directory split.

## Non-Goals

- Do not introduce an `include/` tree.
- Do not enforce one header per `.cpp`.
- Do not split headers for C++ interface purity alone.
- Do not change lowering semantics, testcase expectations, or BIR output.
- Do not add named-case shortcuts or expectation rewrites as proof of progress.

## Working Model

- Headers directly under `src/backend/bir/` are public to `src/**` callers.
- Headers under `src/backend/bir/lir_to_bir/` are private to the LIR-to-BIR
  implementation area unless explicitly exported through a top-level header.
- Headers under `src/backend/bir/lir_to_bir/memory/` are private to the memory
  lowering subdomain.

## Execution Rules

- Keep each code-changing step behavior-preserving.
- Prefer source moves, include updates, and declaration moves over semantic
  rewrites.
- If semantic cleanup appears necessary, record it for a separate idea unless
  it is required to keep the structural change compiling.
- After each structural step, build `c4c_backend` before handing the slice back.
- Run relevant backend tests after steps that change CMake source lists,
  exported headers, or file layout.

## Step 1: Survey Current Header And Include Boundaries

Goal: establish the current public/private declaration split before moving any
declarations.

Primary target: `src/backend/bir/lir_to_bir.hpp` and includes from
`src/backend/bir/lir_to_bir/*.cpp`.

Actions:

- Inspect declarations in `src/backend/bir/lir_to_bir.hpp` and classify them as
  exported API or private lowering implementation machinery.
- Inspect current includes in `src/backend/bir/lir_to_bir/*.cpp` and external
  callers of `src/backend/bir/lir_to_bir.hpp`.
- Identify any declarations that must remain visible to external callers.
- Record the intended Step 2 declaration moves in `todo.md` rather than editing
  the source idea.

Completion check:

- The executor can name the exported declarations that stay in
  `src/backend/bir/lir_to_bir.hpp`.
- The executor can name the private declarations that should move to
  `src/backend/bir/lir_to_bir/lowering.hpp`.
- No implementation files or build files are changed in this step unless the
  supervisor explicitly expands the packet.

## Step 2: Create The LIR-To-BIR Private Directory Index

Goal: split the public entry header from private lowering declarations.

Primary target: `src/backend/bir/lir_to_bir/lowering.hpp`.

Actions:

- Add `src/backend/bir/lir_to_bir/lowering.hpp` as the private index for shared
  lowering implementation declarations.
- Move private lowering context, analysis structs, `lir_to_bir_detail`, and
  `BirFunctionLowerer` machinery out of `src/backend/bir/lir_to_bir.hpp`.
- Keep public options, notes, result types, and entry functions in
  `src/backend/bir/lir_to_bir.hpp`.
- Update `src/backend/bir/lir_to_bir/*.cpp` includes to use `lowering.hpp`
  where private machinery is needed.

Completion check:

- External callers can include `src/backend/bir/lir_to_bir.hpp` without pulling
  in full private lowering declarations.
- LIR-to-BIR implementation files compile through the private
  `lowering.hpp` index.
- `c4c_backend` builds.

## Step 3: Validate The Public BIR Header Surface

Goal: keep top-level BIR declarations centralized without recreating thin
single-purpose headers.

Primary target: `src/backend/bir/bir.hpp`.

Actions:

- Confirm printer and validation declarations remain in `src/backend/bir/bir.hpp`
  when they are BIR-level APIs.
- Remove or avoid reintroducing thin headers such as `bir_printer.hpp` or
  `bir_validate.hpp`.
- Update includes only where needed to preserve the top-level BIR index model.

Completion check:

- The number of thin single-purpose headers in `src/backend/bir` decreases or
  stays flat.
- `c4c_backend` builds.
- Relevant backend tests that cover BIR printing and validation still pass.

## Step 4: Split Memory Lowering Into A Subdirectory

Goal: make memory lowering a semantic subdomain only after the public/private
LIR-to-BIR boundary is clean.

Primary target: `src/backend/bir/lir_to_bir/memory/`.

Actions:

- Move memory-family implementation files under
  `src/backend/bir/lir_to_bir/memory/`.
- Rename `memory.cpp` to `memory/coordinator.cpp` if it remains the opcode
  dispatch/coordinator surface.
- Update CMake source lists and includes for the moved files.
- Add `memory/memory.hpp` only if memory-specific shared declarations should be
  separated from the broader `lowering.hpp` index.

Completion check:

- Memory lowering sources live under a clear memory subdomain directory.
- Memory agents can use one memory index header if such shared declarations are
  needed.
- `c4c_backend` builds.
- Relevant backend memory-lowering tests pass.

## Step 5: Final Structural Validation

Goal: prove the layout refactor preserved behavior.

Actions:

- Inspect the final header set for one-header-per-directory semantics.
- Confirm no new thin one-off headers were introduced.
- Run the supervisor-selected broader backend validation, escalating beyond
  narrow tests if multiple structural slices have landed.
- Record any semantic cleanup discovered during the refactor as separate
  follow-up work instead of folding it into this structural idea.

Completion check:

- Acceptance criteria from the source idea are satisfied.
- `c4c_backend` builds.
- Relevant backend tests pass under the supervisor-selected validation scope.
