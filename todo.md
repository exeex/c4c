# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-2 first legality invariant activation

# Current Packet

## Just Finished
- made the first semantic-BIR legality invariant explicit with
  `PreparedBirInvariant::NoTargetFacingI1`, and taught `run_legalize` to
  publish that invariant when prepare-owned `i1` promotion is part of the
  current target contract
- tightened backend prepare tests so they prove both the explicit invariant
  metadata and the concrete post-legalize effect that target-facing `i1`
  values are removed from prepared semantic BIR in this slice

## Suggested Next
- extend prepare-owned legality invariants beyond `i1` promotion by
  inventorying which target-facing value and call forms still rely on implicit
  target assumptions, then encode the next invariant where legalize already
  owns the transformation
- keep the next packet on general prepared-BIR legality ownership, not target
  ingestion work or testcase-shaped backend shortcuts

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
