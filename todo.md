Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Grouped Width-Greater-Than-One Allocation Truthful
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Step 3 now publishes truthful grouped call-boundary move authority for grouped
width-greater-than-one values across call arguments, call results, and returns.
Prepared call-boundary move records and ABI bindings now carry destination
contiguous width plus occupied ABI register spans instead of collapsing grouped
destinations to scalar-only base-register metadata, and the focused backend
proof covers grouped RISC-V argument/result/return cases through regalloc and
prepared move-bundle publication.

## Suggested Next

Use the new grouped call-boundary authority to close the next remaining Step 3
truthfulness gap in downstream consumers or call/storage plans that still read
only scalar ABI destination metadata.

## Watchouts

- Keep grouped-width consumers keyed to destination span metadata
  (`destination_contiguous_width` and `destination_occupied_register_names`)
  instead of reconstructing grouped ABI lanes from scalar base-register names.
- Keep grouped call-boundary truth anchored to prepared authority rather than
  target-local heuristics or testcase-shaped register-name reconstruction.
- Do not reopen out-of-SSA follow-on work from idea 90 inside this runbook.
- Reject testcase-shaped shortcuts; grouped-width progress must generalize
  beyond one named grouped call-boundary case.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Result: passed after publishing grouped call-boundary destination span authority
in regalloc move records and prepared ABI bindings, with grouped argument,
result, and return coverage added to `backend_prepare_liveness_test`.
Log: `test_after.log`
