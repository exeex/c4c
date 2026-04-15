# Prepare Pipeline Rebuild From Semantic BIR

Status: Active
Source Idea: ideas/open/48_prepare_pipeline_rebuild.md
Source Plan: plan.md
Current Plan Focus: step-2 first legality invariant activation

# Current Packet

## Just Finished
- taught prepare-owned semantic-BIR legalize to repair promoted-type
  bookkeeping for function signatures, local/global storage metadata, and call
  return type text alongside the earlier `i1` value/type and call ABI
  promotion, so the active legality contract no longer leaves those
  target-facing metadata surfaces stale at `i1`/1-byte state
- extended the same `NoTargetFacingI1` legality transform to repair
  load/store alignment plus memory-address size/alignment bookkeeping for
  local and global accesses, so prepared semantic BIR no longer keeps those
  access metadata surfaces at pre-legalized `i1`/1-byte state after scalar
  promotion
- tightened prepare tests so they structurally prove local/global
  load/store-access bookkeeping promotes with the legalized scalar type and
  keep the prepare entry note honest about the expanded prepare-owned memory
  legality contract
- tightened backend prepare tests so they prove the legalized signature and
  storage bookkeeping, require call return type text to move to `i32`, and
  keep the legalize note honest about the prepare-owned contract expansion

## Suggested Next
- inspect the remaining prepare-owned legality surfaces that still only change
  partially under `NoTargetFacingI1`, especially call result ABI/storage
  bookkeeping beyond plain scalar promotion and any remaining prepared-BIR
  metadata that still derives legality implicitly instead of publishing a
  concrete prepare-owned invariant
- keep the next packet on prepared-BIR legality ownership and explicit
  prepared-route contract, not target ingestion work or testcase-shaped
  shortcuts

## Watchouts
- do not let the current legality packet drift into target ingestion work that
  belongs to `ideas/open/49_prepared_bir_target_ingestion.md`
- do not push legality or prepared-phase ownership back into semantic lowering
- keep legality invariants prepare-owned and target-neutral where possible;
  avoid replacing them with target-side assumptions or note-only prose
- signature/storage bookkeeping and call return type text now move with the
  existing `NoTargetFacingI1` promotion, and local/global load-store access
  metadata now moves with it too; any follow-up invariant expansion should
  still be tied to a concrete prepare-owned transform rather than a
  metadata-only promise
- prefer `build -> ^backend_` proof unless a narrower honest backend subset is
  clearly available

## Proof
- delegated proof: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^backend_'`
- passed and wrote `test_after.log`
