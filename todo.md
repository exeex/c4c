Status: Active
Source Idea Path: ideas/open/prealloc-agent-index-header-hierarchy-and-rust-reference-removal.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Decide Exported Top-Level Prealloc Surfaces

# Current Packet

## Just Finished

Completed Step 1: Decide Exported Top-Level Prealloc Surfaces.

Classification results:

- `src/backend/prealloc/prealloc.hpp`: exported surface. It remains the main
  prealloc model and phase API because cross-boundary users include
  `src/backend/backend.hpp`, x86 MIR surfaces under `src/backend/mir/x86/`,
  prealloc implementation files, the stack-layout private index, and backend
  tests.
- `src/backend/prealloc/prepared_printer.hpp`: exported surface, not a current
  merge candidate. It is thin, but it exposes the prepared-module debug printer
  used outside prealloc by `src/backend/backend.cpp`,
  `src/backend/mir/x86/module/module.cpp`, and backend tests. Keeping it
  separate avoids making `prealloc.hpp` also own presentation/debug rendering.
- `src/backend/prealloc/target_register_profile.hpp`: exported surface, not a
  private-move candidate. It is used by prealloc register allocation and
  `prealloc.cpp`, and backend handoff/liveness tests directly exercise its
  ABI/register-pool helpers. No non-test production user outside prealloc was
  found, but the helpers form a target-profile query surface rather than
  stack-layout-private state.

## Suggested Next

Execute Step 2: Rename the Stack-Layout Private Index.

Rename `src/backend/prealloc/stack_layout/support.hpp` to a semantic nested
index such as `src/backend/prealloc/stack_layout/stack_layout.hpp`, then update
the local includes.

## Watchouts

- Keep top-level `prealloc.hpp`, `prepared_printer.hpp`, and
  `target_register_profile.hpp` exported for now.
- Do not merge `prepared_printer.hpp` into `prealloc.hpp` unless a later route
  deliberately folds debug printing into the model API.
- Treat `target_register_profile.hpp` as a target register helper surface, not
  stack-layout-private state.
- Do not delete `ref/claudes-c-compiler/**`.
- Do not change backend semantics or testcase expectations.
- Do not introduce one header per `.cpp`.

## Proof

No build proof required. This packet only inspected include users and updated
`todo.md`; `test_after.log` was not produced or modified.
