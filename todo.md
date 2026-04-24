Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Make Grouped Call-Boundary Consumers Read Published Span Authority
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 3 now carries truthful grouped call-boundary span authority through
derived call plans instead of stopping at regalloc move bundles. Prepared call
argument destinations and call-result ABI sources now preserve contiguous width
plus occupied ABI register spans in the shared call-plan surface, and the
prepared printer shows those grouped spans directly for grouped RISC-V
argument/result fixtures.

## Suggested Next

Advance Step 3.1 by tightening the next downstream grouped call-boundary
consumer surface that still treats published span metadata as scalar
base-register identity instead of reading the shared call-plan span directly.

## Watchouts

- Keep grouped-width consumers keyed to destination span metadata
  (`destination_contiguous_width` and `destination_occupied_register_names`)
  or the corresponding call-plan span fields instead of reconstructing grouped
  ABI lanes from scalar base-register names.
- Keep grouped call-boundary truth anchored to prepared authority rather than
  target-local heuristics or testcase-shaped register-name reconstruction.
- Do not reopen out-of-SSA follow-on work from idea 90 inside this runbook.
- Reject testcase-shaped shortcuts; grouped-width progress must generalize
  beyond one named grouped call-boundary case.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Result: passed after carrying grouped call-boundary span authority into shared
call plans and prepared printer surfaces for grouped argument/result fixtures.
Log: `test_after.log`
