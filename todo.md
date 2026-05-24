Status: Active
Source Idea Path: ideas/open/prealloc-call-plan-phase-split.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Align Prepared Printer Calls Mirror

# Current Packet

## Just Finished

Completed `plan.md` Step 5 prepared-printer call mirror audit: compared
`src/backend/prealloc/prepared_printer/calls.cpp` against the call-plan helper
families extracted in Steps 2-4 and made no printer code change.

Changed files:
- `todo.md`

No-code decision:
- The printer still mirrors the aggregate `PreparedCallPlan` data families
  directly: call header fields, indirect callee detail, memory-return detail,
  argument source/destination fields, result source/destination fields,
  preserved values, and clobbered registers.
- Step 2 extracted argument source and destination construction helpers, but
  did not rename or reshape `PreparedCallArgumentPlan` fields.
- Step 3 extracted clobber span append/deduplication behind a file-local
  helper, but did not change `PreparedClobberedRegister` fields or clobber dump
  meaning. Preservation and boundary effects kept their existing data-family
  boundaries.
- Step 4 extracted call-result construction behind a file-local helper while
  keeping `PreparedCallResultPlan`, `PreparedMemoryReturnPlan`, and formal
  publication data independent.
- Because the data contract and printed field meanings did not change, no
  prepared-printer label, grouping, or helper rename was required.
- Left `src/backend/prealloc/prepared_printer/calls.cpp`,
  `src/backend/prealloc/call_plans.cpp`, public headers, tests, `plan.md`, and
  the source idea untouched.

## Suggested Next

Proceed to `plan.md` Step 6 for the final call-plan boundary review. Confirm
the active diff remains behavior-preserving, summarize the final helper-family
boundaries, and decide whether the source idea is ready for lifecycle closure.

## Watchouts

- Keep this slice behavior-preserving.
- Do not change ABI policy, argument/result placement, clobbering,
  preservation, indirect-callee handling, memory-return behavior, formal
  publication behavior, or prepared dump meaning.
- Keep `calls.hpp` as the aggregate public contract unless usage proves a
  smaller independently consumed boundary.
- The prepared printer is intentionally data-shape-driven; do not rewrite
  labels or grouping for cosmetic alignment with file-local helper names.
- `build_memory_return_plan(...)`, `build_call_result_plan(...)`,
  `plan_call_argument_destination(...)`, `plan_call_argument_source(...)`, and
  `build_call_clobber_set(...)` are construction boundaries only; they did not
  create new public dump sections.
- Step 6 should focus on final route quality, proof sufficiency, and closure
  readiness rather than opening another extraction unless a real ownership
  gap is found.

## Proof

Ran delegated no-code proof:
`git diff --check`

Result: passed.

Proof log: none; no code changed, so the delegated proof did not produce
`test_after.log`.
