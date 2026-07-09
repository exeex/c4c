Status: Active
Source Idea Path: ideas/open/643_rv64_scalar_call_boundary_freshness_after_call.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh The Residual Row

# Current Packet

## Just Finished

Activation packet created the active runbook and initialized execution state
for Step 1.

## Suggested Next

Execute Step 1: refresh the `src/ipa-sra-2.c` RV64 residual row and record the
current first owner with artifact paths.

## Watchouts

- Reject named-case fixes for `src/ipa-sra-2.c`, `calloc`, `foo`, `argc`, or
  physical register `a0`.
- Do not treat a pre-call register home as fresh after a call without explicit
  preservation, republication, or rematerialization authority.
- Do not weaken call-clobber, preservation, republication, rematerialization,
  expectation, unsupported-marker, allowlist, timeout, or pass/fail accounting
  behavior.

## Proof

Lifecycle-only activation; no build or runtime validation run.
