# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-2 first legality invariant activation

# Current Packet

## Just Finished
- extended prepare-owned legality invariants with
  `PreparedBirInvariant::NoPhiNodes`, and taught `run_legalize` to publish
  that invariant alongside `NoTargetFacingI1` so prepared semantic BIR
  exposes phi removal as part of the current contract
- tightened backend prepare tests so they prove both invariant metadata and
  the concrete post-legalize effect that prepared semantic BIR contains no raw
  phi nodes while still removing target-facing `i1` values in the same slice

## Suggested Next
- inventory the remaining target-facing call and value forms that backend
  routing still treats as implicit assumptions, and encode the next
  prepare-owned legality invariant only where `legalize` already owns a real
  transformation rather than target-side fallback
- keep the next packet on prepared-BIR legality ownership and backend-route
  contract clarity, not target ingestion work or testcase-shaped shortcuts

## Watchouts
- do not let the current legality packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push legality or prepared-phase ownership back into semantic lowering
- keep legality invariants prepare-owned and target-neutral where possible;
  avoid replacing them with target-side assumptions or note-only prose
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_'`
- passed and wrote `test_after.log`
- supervisor regression check:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
  reported `70 -> 70` with no new failing tests, but still returned failure
  because the current guard script requires a strict pass-count increase
