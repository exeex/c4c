Status: Active
Source Idea Path: ideas/open/prealloc-call-plan-phase-split.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Call Plan Boundary Review

# Current Packet

## Just Finished

Completed `plan.md` Step 6 final call-plan boundary review: compared the
phase-split diff against `plan.md` and
`ideas/open/prealloc-call-plan-phase-split.md`, including the source idea
acceptance criteria and reject signals.

Changed files:
- `src/backend/prealloc/call_plans.cpp`
- `todo.md`

Final boundary summary:
- `call_plans.cpp` is better structured by durable call-plan subphase:
  argument destination construction is owned by
  `plan_call_argument_destination(...)`, argument source construction by
  `plan_call_argument_source(...)`, clobber span append/deduplication by
  `append_call_clobbered_register_spans(...)`, and result construction by
  `build_call_result_plan(...)`.
- Memory-return handling remains in `build_memory_return_plan(...)`, which
  already owns the sret storage/frame-slot boundary. Formal publication remains
  independent because no call-plan responsibility moved into publication
  storage or naming.
- Preservation and boundary-effect logic were audited and intentionally kept at
  their existing helper/data-family boundaries; moving them would require
  broader mutable planning state without a clearer file-local API.
- `calls.hpp` remains the aggregate public call-plan contract. No smaller
  independently consumed public boundary was proven, and no public headers were
  changed.
- `prepared_printer/calls.cpp` needed no update because the prepared data shape,
  field names, labels, and printed meanings did not change.
- No ABI policy, argument/result placement, clobber set, preservation,
  indirect-callee, memory-return, formal-publication, or prepared dump semantic
  change was found in the final diff.
- No target-shaped shortcut, named-case handling, expectation rewrite, or
  testcase-overfit route was introduced.

## Suggested Next

Recommend supervisor handoff to the plan owner for lifecycle closure of
`ideas/open/prealloc-call-plan-phase-split.md`.

## Watchouts

- Deferred follow-up candidates are not blockers for this source idea:
  `calls.hpp` can stay aggregate until an independently consumed public
  contract exists, and prepared-printer grouping should remain driven by real
  prepared data-family changes rather than file-local helper names.
- If future work revisits preservation or call-boundary move classification, it
  should start as a separate initiative because this runbook's behavior-
  preserving file-local boundaries are exhausted.

## Proof

Step 6 proof:
`git diff --check`

Result: passed.

Earlier code-changing packets in this runbook ran the delegated backend proof
and passed `162/162` backend tests:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Proof log for Step 6: none; this packet made no code change and was delegated
not to create `test_after.log`.
