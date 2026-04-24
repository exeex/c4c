Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Make Grouped Call-Boundary Consumers Read Published Span Authority
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Step 3.1 is now complete. This packet audited the remaining grouped
call-boundary consumer/helpers and owned backend proofs after the selector-seam
repair, confirmed the grouped x86 consumer/module/debug and prepared/regalloc
proof surfaces now read published span authority directly, and found no owned
grouped boundary case still depending on scalar ABI/base-register identity or
lane reconstruction.

## Suggested Next

Advance to Step 3.2 and audit the first width-aware grouped allocation decision
that still behaves like a scalarized placeholder instead of a truthful
contiguous-span allocator choice.

## Watchouts

- Grouped call-argument authority is published on the argument destination span,
  while grouped call-result authority is published on the result source span;
  do not expect result destination-home span duplication when storage-plan
  identity already owns that side.
- Keep grouped call-boundary consumers anchored to published call-plan span
  fields rather than target-local ABI helper callbacks or scalar base-register
  heuristics.
- Step 3.1 closure here is limited to the owned grouped call-boundary consumer
  surface; Step 3.2 still needs a fresh audit for grouped allocation decisions
  outside the already-published call-plan seam.
- Do not reopen out-of-SSA follow-on work from idea 90 inside this runbook.
- Reject testcase-shaped shortcuts; grouped-width progress must generalize
  beyond one named grouped call-boundary case.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Result: passed after the final Step 3.1 audit confirmed no remaining owned
grouped call-boundary consumer/proof still depends on scalar register-name or
base-register reconstruction; the grouped x86 consumer/module/debug surfaces and
grouped prepared/regalloc proofs remain anchored to published span records and
span summaries.
Log: `test_after.log`
