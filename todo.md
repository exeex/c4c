# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh AArch64 Inventory Evidence

# Current Packet

## Just Finished

Lifecycle activation created this active inventory runbook from
`ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md`.

## Suggested Next

Start Step 1 by refreshing or validating the current AArch64 backend
c-testsuite inventory after the closed LR preservation and scalar call-value
routes. Record the current counts and evidence here before selecting any
focused repair family.

## Watchouts

- This umbrella route is planning and classification only; do not implement
  compiler, runner, or test expectation changes here.
- Use explicit timeout behavior and stale generated-runtime process checks for
  any broad runtime scan.
- Do not treat old 46/49/87/7/23 counts as current without checking whether
  post-repair scan evidence still supports them.
- Split and switch to a focused idea before implementation work begins.

## Proof

Lifecycle-only activation. No build, ctest, or regression proof was required.
