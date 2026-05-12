Status: Active
Source Idea Path: ideas/open/188_lir_bir_freeze_closure_gate.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Closure Decision

# Current Packet

## Just Finished

Plan-owner close review for Step 4 rejected closure for idea 188.

Decision: `close rejected`.

Evidence:

- Close-time regression guard passed against the existing canonical full-suite
  logs: before `3137/3137`, after `3137/3137`, no new failures.
- The source idea now lists open dependencies
  `ideas/open/190_lir_call_argument_structured_payload_boundary.md`,
  `ideas/open/191_bir_function_signature_byval_metadata_text_retirement.md`,
  and `ideas/open/194_bir_global_memory_provenance_linknameid_expansion.md`.
- The source idea's in-scope text says the gate must review completed ideas
  183-187, 189-191, and 194, but 190/191/194 are still open and describe
  remaining pre-backend-restart string-authority work.
- The existing `backend restart clear` recommendation therefore proves only
  the older runbook scope, not the current source-idea completion criteria.

## Suggested Next

Supervisor should route lifecycle repair or execution for the updated closure
scope before trying to close idea 188 again. The exact blocker is the source
idea ambiguity between `backend restart clear` and the open 190/191/194
pre-backend-restart blockers.

## Watchouts

- `plan.md` still reflects the older 183-187/189 closure route; the source idea
  is broader and should be treated as authoritative.
- Do not move idea 188 to `ideas/closed/` while 190/191/194 remain open unless
  the supervisor explicitly narrows the source idea or records why those open
  blockers no longer gate backend restart.

## Proof

Plan-owner close review did not create or modify proof logs.

Close-time regression guard command:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Recorded result: `PASS`; before `3137/3137`, after `3137/3137`, no new
failures. Regression status is not the blocker; source-idea completion is.
