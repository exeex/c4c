# Prealloc Agent Index Header Hierarchy And Rust Reference Removal

## Intent

Apply the LLM-agent-oriented header hierarchy strategy to
`src/backend/prealloc`, and remove the reference-only Rust files that remain
from the C++ port.

The goal is to make prealloc easier for agents to navigate by using path depth
as the visibility signal and by eliminating stale reference files that agents
can accidentally read as active implementation.

## Rationale

`src/backend/prealloc` is an active C++ subsystem, but it still contains
reference-only `.rs` files beside the active `.cpp` implementation:

```text
src/backend/prealloc/liveness.rs
src/backend/prealloc/regalloc.rs
src/backend/prealloc/stack_layout/*.rs
```

Those Rust files are not part of the active build. Their original purpose was
porting guidance, but their continued presence increases search noise and
token cost. An agent looking for the current implementation can easily open a
reference `.rs` file and waste context or reason from stale behavior.

Prealloc also has a header shape that should be made explicit for agents:

- top-level headers are exported/public surfaces for `src/**`
- nested headers are private indexes for implementation subdomains
- thin one-off headers should be merged into an appropriate index unless they
  intentionally export a surface

## Visibility Rule

Use path depth as the primary visibility signal:

```text
src/backend/prealloc/*.hpp
  public or exported prealloc surfaces for src/** callers

src/backend/prealloc/stack_layout/*.hpp
  private to the stack-layout implementation subdomain
```

Nested headers are private by default because agents only reach them after
entering that semantic area. Top-level headers are public by default because
they are visible at the prealloc package boundary. If an interface must be
exported across a wider boundary, it should have an explicit top-level `.hpp`.

## Design Direction

Use this rule:

```text
directory = semantic boundary
.hpp      = index entry for that boundary
```

Avoid one header per `.cpp`. Prefer one meaningful header per semantic
directory. Do not keep reference-only files beside active implementation once
the C++ port is the source of truth.

## Current Hotspots

Current notable files:

- `src/backend/prealloc/prealloc.hpp`
  - large public prealloc data model and phase API surface
- `src/backend/prealloc/prealloc.cpp`
  - main pipeline coordinator
- `src/backend/prealloc/legalize.cpp`
  - BIR legalization into prepared form
- `src/backend/prealloc/liveness.cpp`
  - liveness analysis implementation
- `src/backend/prealloc/out_of_ssa.cpp`
  - phi/materialization and SSA exit handling
- `src/backend/prealloc/regalloc.cpp`
  - register allocation and spill assignment
- `src/backend/prealloc/prepared_printer.hpp`
  - thin printer declaration header
- `src/backend/prealloc/target_register_profile.hpp`
  - exported target register profile surface
- `src/backend/prealloc/stack_layout/support.hpp`
  - private stack-layout helper/index surface, currently named generically
- `src/backend/prealloc/**/*.rs`
  - reference-only Rust files retained during porting

## Target Shape

Preferred long-term layout:

```text
src/backend/prealloc/
  prealloc.hpp
  prealloc.cpp
  prepared_printer.cpp
  target_register_profile.hpp
  target_register_profile.cpp
  legalize.cpp
  liveness.cpp
  out_of_ssa.cpp
  regalloc.cpp

  stack_layout/
    stack_layout.hpp
    coordinator.cpp
    analysis.cpp
    alloca_coalescing.cpp
    copy_coalescing.cpp
    inline_asm.cpp
    regalloc_helpers.cpp
    slot_assignment.cpp
```

The exact filenames can change during planning, but the hierarchy should keep
the public/private signal simple:

- `prealloc.hpp` is the exported prealloc model and phase API
- `target_register_profile.hpp` remains exported if callers outside prealloc
  need that model
- `prepared_printer.hpp` should either remain explicitly exported or merge
  into `prealloc.hpp` as a small prealloc-level API
- `stack_layout/stack_layout.hpp` is the private index for stack-layout
  implementation helpers
- no `.rs` files should remain under `src/backend/prealloc`

## Refactoring Steps

1. Decide and document top-level exported prealloc surfaces.
   - Keep `prealloc.hpp` as the main public prealloc API/data model.
   - Keep `target_register_profile.hpp` public if its callers are broader than
     prealloc implementation.
   - Decide whether `prepared_printer.hpp` should merge into `prealloc.hpp` or
     stay as an explicit exported printer surface.

2. Rename the stack-layout private index.
   - Replace `src/backend/prealloc/stack_layout/support.hpp` with a clearer
     directory-level index such as
     `src/backend/prealloc/stack_layout/stack_layout.hpp`.
   - Update stack-layout `.cpp` files and any legitimate prealloc callers to
     include the renamed private index.
   - Do not create per-file headers for stack-layout implementation files.

3. Move the stack-layout coordinator if useful.
   - Consider moving `src/backend/prealloc/stack_layout.cpp` to
     `src/backend/prealloc/stack_layout/coordinator.cpp`.
   - Keep public orchestration APIs declared through `prealloc.hpp`.
   - Keep implementation-only helpers behind the nested stack-layout index.

4. Remove reference-only Rust files from active source.
   - Delete `src/backend/prealloc/liveness.rs`.
   - Delete `src/backend/prealloc/regalloc.rs`.
   - Delete `src/backend/prealloc/stack_layout/*.rs`.
   - Update `README.md` and comparison notes that claim `.rs` files remain as
     local references.
   - Do not remove `ref/claudes-c-compiler/**` in this idea; that external
     reference tree is a separate archive policy question.

5. Preserve behavior.
   - This initiative is structural and cleanup-oriented.
   - It should not change prealloc semantics, stack layout behavior, register
     allocation behavior, printer output, or testcase expectations.
   - Any semantic differences discovered while deleting Rust references should
     become separate ideas unless required to keep current C++ behavior
     documented.

## Acceptance Criteria

- No `.rs` files remain under `src/backend/prealloc`.
- Prealloc agents can treat top-level `*.hpp` files as exported surfaces.
- Stack-layout agents can open one nested private index header under
  `src/backend/prealloc/stack_layout/`.
- Thin headers are reduced or explicitly justified as exported surfaces.
- No one-header-per-`.cpp` pattern is introduced.
- `c4c_backend` builds after each structural slice.
- Relevant backend/prealloc tests continue to pass.
- Prealloc README/comparison docs no longer instruct agents to consult deleted
  local Rust reference files as active guidance.

## Non-Goals

- Do not introduce a traditional separated `include/` tree.
- Do not use headers primarily to enforce C++ interface purity.
- Do not split every `.cpp` into a matching `.hpp`.
- Do not change stack-layout, liveness, out-of-SSA, or regalloc semantics as
  part of this layout cleanup.
- Do not delete the broader `ref/claudes-c-compiler/**` Rust archive as part
  of this prealloc-local cleanup.
