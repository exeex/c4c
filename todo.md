Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Make Grouped Call-Boundary Consumers Read Published Span Authority
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Step 3.1 now makes the grouped x86 call-boundary proof read published span
authority directly for the remaining selector seam. The grouped consumer-surface
contract now proves the argument/result selectors return the published prepared
call-plan records themselves, and the grouped module-emitter and route-debug
proofs share span-summary helpers instead of rebuilding those expectations from
scalar `*_register_name` checks.

## Suggested Next

Audit the remaining x86 grouped call-boundary helpers and proofs for any last
scalar ABI/base-register phrasing outside this packet; if none remain, hand
Step 3.1 back for closure rather than widening into a new family here.

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
Result: passed after moving the remaining grouped call-boundary selector proof
off scalar register-name presence checks and onto direct published span-record
identity plus shared grouped span summaries across the x86 consumer/module/debug
proof surfaces.
Log: `test_after.log`
