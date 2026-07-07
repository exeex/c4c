Status: Active
Source Idea Path: ideas/open/580_rv64_scalar_compare_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce Compare Publication Owners

# Current Packet

## Just Finished

Activation created a fresh runbook from `ideas/open/580_rv64_scalar_compare_publication.md`.

## Suggested Next

Execute `plan.md` Step 1 by confirming current scalar compare publication owner
facts for at least one simple compare row and one select-consuming compare row.
Use existing `550` salvage evidence where sufficient and save fresh artifacts
under `build/agent_state/580_rv64_scalar_compare_publication/step1/` when reruns
are needed.

## Watchouts

- Keep this lane limited to ordinary F32/F64 compare result publication into
  prepared GPR homes.
- Do not mix in floating casts, variadic helpers, F128, long-double, or helper
  implementation work.
- Do not claim progress through unsupported-marker changes, expectation
  rewrites, route allowlist edits, named-case checks, or fixed-register
  shortcuts.

## Proof

Lifecycle-only activation; no build or route proof required.
