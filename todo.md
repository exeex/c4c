Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Make Grouped Call-Boundary Consumers Read Published Span Authority
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 3.1 now makes the x86 module-emitter comments and route-debug trace report
grouped call-argument destination spans and grouped call-result source spans
directly from prepared call-plan authority. The grouped cross-call proof now
checks those consumer-facing summaries alongside the existing
saved/preserved/clobber/storage reporting so the call-boundary surface stays
anchored to published span metadata.

## Suggested Next

Advance Step 3.1 by moving the next remaining x86 call-boundary helper or dump
surface that still talks in scalar ABI register terms onto the prepared grouped
argument/result span fields directly, or close Step 3.1 if no honest consumer
surface remains beyond proof tightening.

## Watchouts

- Grouped call-argument authority is published on the argument destination span,
  while grouped call-result authority is published on the result source span;
  do not expect result destination-home span duplication when storage-plan
  identity already owns that side.
- Keep grouped call-boundary consumers anchored to published call-plan span
  fields rather than target-local ABI helper callbacks or scalar base-register
  heuristics.
- Do not reopen out-of-SSA follow-on work from idea 90 inside this runbook.
- Reject testcase-shaped shortcuts; grouped-width progress must generalize
  beyond one named grouped call-boundary case.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Result: passed after teaching the x86 module-emitter comments and route-debug
surface to report grouped call argument/result spans directly from prepared
call-plan authority, with focused contract coverage for both grouped
call-boundary and grouped spill/reload summaries.
Log: `test_after.log`
