Status: Active
Source Idea Path: ideas/open/188_lir_bir_freeze_closure_gate.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Dependency Readiness Check

# Current Packet

## Just Finished

Plan-owner repair rewrote the active runbook after close rejection for idea 188.

The older closure route covered ideas 183-187 plus 189 and produced a stale
`backend restart clear` recommendation. That is no longer sufficient because
the authoritative source idea now lists open dependencies 190, 191, and 194 and
requires the gate to review completed ideas 189-191 and 194.

Decision: keep idea 188 active, but treat the closure gate as dependency-blocked
until 190, 191, and 194 are closed or the source idea is explicitly narrowed by
lifecycle action.

## Suggested Next

Execute Step 1 from `plan.md`: confirm the open dependency status and route
lifecycle work to the remaining blockers before another close attempt.

Current known blocker order:
- `ideas/open/190_lir_call_argument_structured_payload_boundary.md`
- `ideas/open/191_bir_function_signature_byval_metadata_text_retirement.md`
- `ideas/open/194_bir_global_memory_provenance_linknameid_expansion.md`

Idea 191 depends on 190, so 190 should be handled before 191. Idea 194 is an
independent remaining global-provenance blocker for the freeze gate.

## Watchouts

- Do not mark `backend restart clear` from the existing full-suite regression
  proof alone; regression status was green, but source-idea completion was
  false.
- Do not move idea 188 to `ideas/closed/` while 190/191/194 remain open unless
  the supervisor explicitly delegates a source-idea narrowing or blocker
  reclassification.
- Do not start backend restart implementation inside this gate.

## Proof

Lifecycle repair only. No implementation validation or proof logs were created
or modified.

Existing close-time regression guard was previously recorded as passing:
before `3137/3137`, after `3137/3137`, no new failures. That proof is not the
blocker and does not clear the updated source-idea dependency scope.
