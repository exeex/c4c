Status: Active
Source Idea Path: ideas/open/550_rv64_scalar_fpr_residual_salvage.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce Current Residual Rows

# Current Packet

## Just Finished

Activation created a fresh runbook from `ideas/open/550_rv64_scalar_fpr_residual_salvage.md`.

## Suggested Next

Execute `plan.md` Step 1 by locating or reproducing the current scalar compare,
floating-cast, and variadic helper residual rows and saving evidence under
`build/agent_state/550_rv64_scalar_fpr_residual_salvage/step1/`.

## Watchouts

- Keep F128 and long-double helper cases quarantined outside this lane.
- Do not claim implementation progress from expectation rewrites, unsupported
  marker changes, or named-case shortcuts.
- Split coherent implementation follow-ups into separate `ideas/open/` files
  instead of widening this classification plan.

## Proof

Lifecycle-only activation; no build or route proof required.
