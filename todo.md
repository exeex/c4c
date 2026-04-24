Status: Active
Source Idea Path: ideas/open/prealloc-agent-index-header-hierarchy-and-rust-reference-removal.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Rename the Stack-Layout Private Index

# Current Packet

## Just Finished

Completed Step 2: Rename the Stack-Layout Private Index.

Renamed `src/backend/prealloc/stack_layout/support.hpp` to
`src/backend/prealloc/stack_layout/stack_layout.hpp` with `git mv`.
Updated the local stack-layout implementation includes and the legitimate
prealloc callers in `stack_layout.cpp` and `regalloc.cpp`.

Verification scan:

- No `*.cpp` or `*.hpp` source under `src/backend/prealloc` includes
  `stack_layout/support.hpp` or `"support.hpp"`.
- `src/backend/prealloc/stack_layout/stack_layout.hpp` is the only nested
  stack-layout private index header.

## Suggested Next

Execute Step 3: Move the Stack-Layout Coordinator If Clean.

Inspect whether `src/backend/prealloc/stack_layout.cpp` can be mechanically
moved to `src/backend/prealloc/stack_layout/coordinator.cpp` without changing
public orchestration declarations or backend behavior.

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
- `src/backend/prealloc/stack_layout/README.md` still mentions the old
  `support.hpp` reference table entry; leave that for the later docs/Rust
  reference cleanup unless the supervisor widens ownership.

## Proof

Proof command:

`bash -lc 'cmake --build --preset default --target c4c_backend' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the canonical proof output for this
packet.
