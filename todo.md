Status: Active
Source Idea Path: ideas/open/715_pass_ready_bir_schema_and_legacy_quarantine_research.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define The Pass-Ready BIR Contract

# Current Packet

## Just Finished

- Completed plan.md Step 3: defined stable identities and storage, construction
  and mutation APIs, terminator-derived CFG, verifier rules, revision-bound
  analyses/invalidation, and safe parallel per-function processing.

## Suggested Next

- Execute plan.md Step 4 and compare the reference backend's ownership, CFG,
  analysis, pass, and phi-removal principles against c4c requirements.

## Watchouts

- Keep persistent semantic IDs distinct from temporary dense analysis indices
  and mutation-scoped raw references.
- Module symbol/type/global/function state is frozen during parallel function
  edits; cross-function mutation requires an exclusive module barrier.

## Proof

- Supervisor-selected documentation proof passed:
  `git diff --check && test -f docs/backend/pass_ready_bir/03_pass_ready_bir_contract.md && rg -n "FunctionId|BlockId|InstId|ValueId|RAUW|split|redirect|verifier|invalidation|terminator|parallel" docs/backend/pass_ready_bir/03_pass_ready_bir_contract.md`.
- This documentation-only packet does not produce `test_after.log`; no build or
  test subset was delegated.
