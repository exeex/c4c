Status: Active
Source Idea Path: ideas/open/08_calls_argument_sources_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Retire Or Reduce The Translation Unit

# Current Packet

## Just Finished

Step 4 retired `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`.

The CMake source list no longer includes the deleted translation unit.
Emission-local helpers moved to their owning files: indirect callee and memory
return storage helpers now live in `calls.cpp`; stack destination, aggregate
source, sret fallback, prior-preservation, and selected source detail helpers
now live in `calls_moves.cpp`; the byval frame-slot lookup is localized in
`calls_byval_aggregates.cpp`.

`calls.hpp` now exposes only the still-shared selection-driven converter,
`make_selected_call_argument_source`, because both direct move lowering and the
dispatch bridge consume prepared source selections.

## Suggested Next

Supervisor can review and commit the Step 4 deletion slice, then decide whether
the active plan is exhausted or needs plan-owner lifecycle handling.

## Watchouts

- `make_selected_call_argument_source` remains exported only because
  `calls_dispatch_bridge.cpp` and `calls_moves.cpp` both still call it.
- `make_sret_memory_return_address_source` is now local to `calls_moves.cpp`;
  it still owns the memory-return fallback path when no ordinary argument
  source selection is present.
- `find_frame_slot_by_id` is no longer exported from `calls.hpp`; `cast_ops.cpp`
  already had its own local lookup, and `calls_byval_aggregates.cpp` now owns
  its byval lookup locally.
- `clang-format` is not installed in this workspace; formatting was kept
  manually in the existing style.

## Proof

Passed; proof log preserved at `test_after.log`.

Delegated command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: build succeeded and all 162 `^backend_` tests passed.
