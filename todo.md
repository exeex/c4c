Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Make Grouped Call-Boundary Consumers Read Published Span Authority
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 3.1 now exposes direct x86 call-boundary selectors for prepared call
plans, arguments, and results keyed by block/instruction identity instead of
forcing downstream consumers to reopen scalar ABI register reconstruction.
The grouped call-boundary contract now proves x86 reads published grouped
argument destination spans and grouped call-result source spans directly for a
grouped cross-call fixture while preserving the existing grouped
preserved/clobber/storage consumer checks.

## Suggested Next

Advance Step 3.1 by tightening the next x86 bounded helper or emitter adapter
that still accepts scalar ABI register callbacks for call argument/result
selection instead of consuming the prepared grouped call-boundary selectors
directly.

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
Result: passed after adding direct x86 prepared call-boundary selectors and
proving grouped argument/result span consumption through the x86 consumer
surface.
Log: `test_after.log`
