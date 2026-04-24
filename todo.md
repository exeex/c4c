Status: Active
Source Idea Path: ideas/open/prealloc-agent-index-header-hierarchy-and-rust-reference-removal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move the Stack-Layout Coordinator If Clean

# Current Packet

## Just Finished

Completed Step 3: Move the Stack-Layout Coordinator If Clean.

Moved `src/backend/prealloc/stack_layout.cpp` to
`src/backend/prealloc/stack_layout/coordinator.cpp` with `git mv`.
Updated the moved coordinator's local includes from top-level prealloc-relative
paths to the stack-layout directory context:

- `prealloc.hpp` is now included as `../prealloc.hpp`.
- `stack_layout/stack_layout.hpp` is now included as `stack_layout.hpp`.
- `src/backend/CMakeLists.txt` did not need editing; its recursive prealloc
  source glob discovered the moved coordinator and reconfigured during the
  proof build.

## Suggested Next

Execute Step 4: Remove Prealloc-Local Rust Reference Files.

Delete the prealloc-local `.rs` reference files under `src/backend/prealloc`
and update prealloc-local docs/comparison notes that point agents at those
deleted local Rust files as active guidance. Do not delete
`ref/claudes-c-compiler/**`.

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
- Several docs and local Rust reference comments still mention
  `stack_layout.cpp`; Step 4 owns the local Rust reference removal and nearby
  documentation cleanup.

## Proof

Proof command:

`bash -lc 'cmake --build --preset default --target c4c_backend' > test_after.log 2>&1`

Result: passed. CMake detected the recursive glob mismatch, reconfigured, built
`prealloc/stack_layout/coordinator.cpp.o`, and relinked `c4c_backend`.
`test_after.log` contains the canonical proof output for this packet.

Baseline review: supervisor accepted `test_baseline.new.log` as
`test_baseline.log`, green for `^backend_` at commit `36442b9d`.
