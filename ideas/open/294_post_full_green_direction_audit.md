# Post Full-Green Direction Audit

## Goal

Decide the next engineering direction after the 286-293 sequence restored the
focused backend routes, closed the prepared/interface cleanup tail, and produced
a full-suite green baseline.

The output should be a concrete direction map, not another automatic cleanup
queue.

## Why This Exists

The repo now has strong evidence that the recent route is healthy:

- `^backend_` reached 180/180 passing after idea 293.
- `test_baseline.log` records a full-suite 3208/3208 passing baseline at
  commit `73543a60537a69f56db3502cef9362eb6edec870`.
- The hook-managed baseline flow is active again.
- `ideas/open/` is empty.

That is a good moment to pause and choose deliberately. Continuing cleanup just
because the last route had cleanup momentum risks inventing work or reopening
retained prepared-policy surfaces without a concrete need.

## In Scope

- Review closure notes from ideas 286 through 293.
- Review current baseline evidence:
  - `test_baseline.log`;
  - recent `^backend_` proof;
  - current open/active lifecycle state.
- Classify potential next work into:
  - stop/defer because the surface is intentionally retained policy;
  - monitor only because current tests are green and no defect is exposed;
  - open a new source idea because a concrete gap remains;
  - start implementation because a bounded, proofable packet is available.
- Check whether any closure note left an explicit follow-up that was not
  already closed or intentionally deferred.
- Produce at most a small number of new follow-up ideas, only if they have
  concrete owner files, proof commands, and reject signals.

## Out of Scope

- Making implementation changes during the audit.
- Retiring prepared/prealloc public surfaces without fresh route/BIR agreement
  and fail-closed proof.
- Reopening 286-293 just to make cosmetic cleanup commits.
- Creating broad "cleanup everything" ideas.
- Treating the full-suite baseline as permission to weaken tests or supported
  path contracts.

## Acceptance Criteria

- The closure note records a concise direction matrix for the post-green state.
- Every generated follow-up idea has:
  - concrete owner surface;
  - proof route;
  - explicit reviewer reject signals;
  - a reason it should exist now rather than be deferred.
- If no follow-up is warranted, the closure says that clearly and leaves
  `ideas/open/` empty.
- The audit confirms whether `test_baseline.log` is accepted and whether
  `test_baseline.new.log` is absent or pending review.
- No implementation files are changed.

## Reviewer Reject Signals

- The route opens cleanup ideas without tying them to a concrete defect,
  interface smell, or closure-note leftover.
- The route treats retained prepared/prealloc policy as automatically obsolete
  because the suite is green.
- The route makes code changes under an audit-only idea.
- The route ignores the full-suite baseline state or hook-managed baseline
  candidate state.
- The route generates broad follow-ups with vague proof commands or generic
  reject signals.
