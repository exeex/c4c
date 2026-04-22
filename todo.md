# Execution State

Status: Active
Source Idea Path: ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reclassify Remaining Backend Failures After The Repair
Plan Review Counter: 0 / 5
# Current Packet

## Just Finished

Step `4` reclassified the current dirty slice to the proved idea-76 repair
only. The accepted progress remains the stack-layout publication change: once
one slice of an address-exposed aggregate-local family requires a fixed home,
`assign_frame_slots(...)` keeps the whole slice family in the fixed-location
group so `%lv.param.a.*`, `%lv.param.b.*`, and adjacent families publish
contiguous homes instead of interleaving only their `.0` slices.

This packet also removed the unproved regalloc byval-home contract experiment
from `regalloc.cpp`, leaving the worktree limited to the publication/layout
repair plus the signature plumbing needed to pass `PreparedNameTables` into
slot assignment. The delegated proof refreshed `test_after.log` for the
current narrowed slice. A plan-owner close review then judged the source idea's
semantic completion signal satisfied, but rejected closure because the required
canonical regression guard on `test_before.log` vs `test_after.log` did not
strictly improve the focused proof scope (`1` pass / `1` fail before and
after).

## Suggested Next

Treat idea 76 as semantically complete for ownership purposes, but keep the
lifecycle open until the supervisor chooses an acceptable closure-grade proof
scope or explicitly deactivates/switches the runbook. The remaining
`00204.c` mismatch is downstream return-value / variadic / HFA work, not more
byval-home publication/layout repair.

## Watchouts

- Keep idea-76 acceptance limited to the generic fixed-slice-family publication
  rule; do not reintroduce byval-home sizing logic in regalloc without proof
  that the remaining failures are still in this seam.
- The callee-side argument corruption stays repaired only as long as
  `assign_frame_slots(...)` continues to publish whole aggregate-local families
  contiguously once any slice in that family requires a permanent home.
- `c_testsuite_x86_backend_src_00204_c` may still fail after this packet; that
  is reclassification evidence, not proof that idea 76 still owns the next bad
  fact.
- Do not mark idea 76 closed from the current focused logs alone: the required
  canonical regression guard returned `FAIL` because pass count stayed flat on
  the chosen two-test scope.

## Proof

Delegated proof command for Step `4`:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`

Latest packet result:

- refreshed `test_after.log` with the narrowed slice after removing the
  premature regalloc byval-home contract experiment
- `backend_x86_handoff_boundary` passed
- `c_testsuite_x86_backend_src_00204_c` still failed, with `Arguments:`
  continuing to match through the earlier multi-aggregate byval probes while
  the mismatch remains in the later return-value / variadic / HFA surface
- use the test result in `test_after.log` as the canonical proof record for
  whether only the stack-layout publication repair remains justified
- plan-owner close gate on 2026-04-22: `close rejected`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
  reported no new failures but also no strict pass-count increase (`1` pass /
  `1` fail before and after), so closure is not yet acceptance-ready on this
  proof scope
