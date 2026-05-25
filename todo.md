Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reactivation Mapping With Prepared Source Selection

# Current Packet

## Just Finished

Step 4 of the shared prepared call-argument source-selection prerequisite
accepted and closed
`ideas/open/01_shared_prepared_call_argument_source_selection.md`.

Acceptance basis:

- Shared `PreparedCallArgumentSourceSelection` exists on prepared call
  argument plans.
- AArch64 consumes complete shared selections for frame-slot values,
  frame-slot addresses, local-frame address materialization, stack-slot prior
  preservation, and complete byval register-lane selections.
- Deferred callee-saved-register preservation and fragmented byval fallback
  paths are outside the accepted prerequisite because the shared selection does
  not yet carry enough target-neutral placement/source payload to retire those
  emission paths.
- The latest accepted backend proof was
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`,
  with 162/162 backend tests passing. `test_before.log` has been rolled
  forward to that accepted proof.

## Suggested Next

Execute Step 1 of `plan.md`: map the remaining AArch64 call helper boundaries
against available prepared call-plan facts, prioritizing routes made actionable
by `PreparedCallArgumentSourceSelection`, then delegate the first coherent
implementation slice if the mapping identifies one.

## Watchouts

- Do not perform another AArch64-local move/source cleanup unless the chosen
  helper boundary maps to prepared facts that now exist.
- Do not invent a new call-plan API under this source idea.
- Do not move AArch64 emission details into the shared planner.
- Do not claim progress through file concatenation, expectation weakening, or
  hidden helper rewrites.
- Callee-saved-register prior-preservation and fragmented byval fallback paths
  still require fresh mapping before they can justify deletion or consolidation.

## Proof

Close gate used the accepted backend proof rolled into `test_before.log`.
Regression guard checker was run read-only against the rolled-forward proof
with non-decreasing mode and reported 162 passed, 0 failed.
