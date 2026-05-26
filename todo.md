Status: Active
Source Idea Path: ideas/open/08_calls_argument_sources_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Shrink The AArch64 Calls Surface

# Current Packet

## Just Finished

Step 3 shrank the exported AArch64 call-argument source helper surface.

`calls.hpp` now exposes one explicit selection-driven converter,
`make_selected_call_argument_source`, for prepared frame-slot value,
frame-slot address, and local-frame-address source facts. The old public
wrappers that reread `PreparedCallArgumentPlan::source_selection` internally
were removed.

The direct AArch64 move and dispatch-bridge callers now branch on their prepared
`source_selection` and pass that selection plus the expected kind into the
converter. The removed local-frame-address wrapper also drops the remaining
absent-selection materialization scan/name-match recovery path; source choice
stays in prepared call facts.

## Suggested Next

Supervisor can review and commit the Step 3 helper-surface shrink. A next
execution packet should focus on any remaining byval/payload absent-selection
route debt only if the active plan still calls for it.

## Watchouts

- `make_sret_memory_return_address_source` still owns the memory-return fallback
  path when no ordinary argument source selection is present.
- `find_frame_slot_by_id` remains shared outside call-argument source selection
  and was intentionally left exported.
- `clang-format` is not installed in this workspace; formatting was kept
  manually in the existing style.

## Proof

Passed; proof log preserved at `test_after.log`.

Delegated command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: build succeeded and all 162 `^backend_` tests passed.
