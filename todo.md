# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Current AArch64 Backend Inventory

# Current Packet

## Just Finished

Activated the umbrella AArch64 c-testsuite failure family inventory as the
single active plan after closing the prior focused idea.

## Suggested Next

Supervisor should delegate Step 1 to refresh or verify the current
`build-aarch64-scan` AArch64 backend c-testsuite inventory with an explicit
timeout, then record the current bucket counts and log locations here.

## Watchouts

- This umbrella plan is inventory and lifecycle routing only; do not implement
  compiler repairs under it.
- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Broad runtime scans must use an explicit timeout and stale generated-runtime
  process cleanup.
- Keep timeout/hang cases separate from ordinary mismatch work unless a
  timeout-specific focused idea is created.
- Split a focused `ideas/open/*.md` repair idea before implementation starts.

## Proof

Lifecycle activation only. No build or test proof was run.
