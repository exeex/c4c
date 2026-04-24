Status: Active
Source Idea Path: ideas/open/prealloc-agent-index-header-hierarchy-and-rust-reference-removal.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Structural Validation and Cleanup

# Current Packet

## Just Finished

Completed Step 4: Remove Prealloc-Local Rust Reference Files.

Deleted all prealloc-local `.rs` reference files under
`src/backend/prealloc`. Updated the prealloc README, stack-layout README, and
phase comparison notes so agents no longer treat those deleted local Rust files
as active guidance. `stack_layout/README.md` now reflects
`stack_layout.hpp` and `coordinator.cpp` as the active C++ stack-layout
surfaces.

## Suggested Next

Step 5: Final Structural Validation and Cleanup.

Re-scan `src/backend/prealloc` headers and docs for the intended visibility
signal, confirm no one-header-per-`.cpp` pattern was introduced, run the
supervisor-selected backend/prealloc proof, and then decide whether this active
plan is ready for lifecycle closure.

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
- Historical Rust material remains available under `ref/claudes-c-compiler/`
  for explicit archaeology, but prealloc-local `.rs` files are no longer an
  active guidance source.

## Proof

Proof command:

`bash -lc 'cmake --build --preset default --target c4c_backend' > test_after.log 2>&1`

Result: passed. Ninja rechecked globbed directories and reported no work to do.
`test_after.log` contains the canonical proof output for this packet.

Baseline review: supervisor accepted `test_baseline.new.log` as
`test_baseline.log`, green for `^backend_` at commit `a2a82934`.
