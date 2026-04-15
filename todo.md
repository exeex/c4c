# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-2 first legality invariant activation

# Current Packet

## Just Finished
- taught prepare-owned semantic-BIR legalize to promote call argument ABI and
  result ABI metadata alongside existing target-facing `i1` value/type
  promotion, so the active `NoTargetFacingI1` contract now covers call ABI
  metadata instead of leaving that surface implicit
- tightened backend prepare tests so they prove the legalized call ABI
  metadata, preserve the existing no-phi/no-target-facing-`i1` contract, and
  require the legalize note to mention call ABI metadata explicitly

## Suggested Next
- inspect the remaining prepare-owned legality metadata surfaces that still
  rely on semantic lowering output after `i1` promotion, especially function
  signature and slot/global size-alignment bookkeeping, and either legalize
  them or leave them out of the prepared contract until `legalize` owns a real
  transform there
- keep the next packet on prepared-BIR legality ownership and explicit
  prepared-route contract, not target ingestion work or testcase-shaped
  shortcuts

## Watchouts
- do not let the current legality packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push legality or prepared-phase ownership back into semantic lowering
- keep legality invariants prepare-owned and target-neutral where possible;
  avoid replacing them with target-side assumptions or note-only prose
- call ABI type promotion now lands under the existing `NoTargetFacingI1`
  invariant; any follow-up invariant should still be tied to a concrete
  prepare-owned transform rather than a metadata-only promise
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_'`
- passed and wrote `test_after.log`
