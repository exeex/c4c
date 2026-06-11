Status: Active
Source Idea Path: ideas/open/197_return_chain_import_and_naming_clarification.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Return-Chain Source Materials

# Current Packet

## Just Finished

Step 1 inventory is complete for the durable return-chain import note.

Stable source list the note must cite:

- `docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md`: accepted
  owner/schema facts. Return-chain owns target-neutral BIR identity only:
  function/block context, instruction position or stable reference, named chain
  value, terminal return value, optional immediate next-operand value, and
  absent/invalid status for unavailable or conflicting answers.
- `ideas/closed/176_return_chain_owner_schema_decision.md`: closed after
  choosing BIR-owned return-chain identity, rejecting permanent prepared
  ownership and target-local AArch64 ownership for the semantic relation, while
  keeping homes/registers/ABI/scratch/emission policy target-local.
- `ideas/closed/177_bir_return_chain_schema_index.md`: closed after adding the
  distinct Route 8 schema/index for same-block return-chain identity with
  fail-closed unsupported and conflict behavior; AArch64 consumers and prepared
  helpers remained unmigrated at that point.
- `ideas/closed/178_bir_return_chain_oracle_equivalence.md`: closed after
  proving BIR route answers equivalent to the prepared terminal and
  next-operand helpers for focused oracle coverage; prepared helpers remained
  public and AArch64 consumers were not yet migrated.
- `ideas/closed/179_bir_return_chain_consumer_migration.md`: closed after
  AArch64 return-chain consumers stopped reading prepared return-chain helper
  facts and read the BIR-owned route instead, with terminal/next-operand proof
  and prepared helper APIs still public for the later contraction.
- `ideas/closed/180_bir_return_chain_prepared_api_contraction.md`: closed after
  removing the public prepared return-chain helper surface from
  `prepared_lookups.hpp`; removed names had no remaining matches under `src`
  or `tests`, and focused plus `^backend_` proof was green.

Current docs references needing precise import wording:

- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md` already
  says return-chain is importable only as its own owner/schema line from the
  decision doc and closed ideas 176-180. Its note that prepared return-chain
  helpers should remain public migration oracles is historically correct for
  the readiness audit point but is stale after idea 180; the new import note
  should say the public helper surface is now absent after contraction.
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
  keeps return-chain outside Route 1, Route 7, predecessor rescans, name
  matching, and generic route-index facades; this is a useful citation hazard
  list for the import note.
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
  references return-chain in route/debug and AArch64 lookup-threading oracle
  contexts. The import note should avoid implying that prepared diagnostics or
  `PreparedFunctionLookups` handles remain authoritative for return-chain
  identity after the Route 8 migration and API contraction.
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
  does not include a live `return_chains` lookup-group row, so the import note
  should present return-chain as already migrated/contracted rather than as one
  of the retained seven `PreparedFunctionLookups` groups.
- `docs/bir_prealloc_fusion/cross_target_route_view_reuse_map.md` names
  `PreparedFunctionLookups` and `PreparedBirModule` cross-target wrapper
  hazards but no return-chain reuse boundary; the import note should not claim
  x86 or riscv wrapper readiness from return-chain closure.

Citation hazards found:

- Do not cite return-chain as Route 1 producer identity or Route 7 comparison
  provenance; the decision doc explicitly rejects both ownership choices.
- Do not describe return-chain as predecessor rescan, name matching, or a
  generic route-index facade. The accepted line is a distinct Route 8
  owner/schema line with typed terminal and next-operand identity.
- Do not import AArch64 value homes, return ABI register choice, before-return
  moves, prepared-register conversion, alias checks, scalar register view,
  scratch selection, final ALU records, or emission order into the
  target-neutral route identity.
- Do not claim public prepared return-chain helper retention as a current
  blocker; idea 180 removed that public surface. Historical docs that predate
  idea 180 should be cited as historical readiness state only.
- Do not use the completed return-chain sequence as broad
  `PreparedFunctionLookups` or `PreparedBirModule` retirement evidence.
  Return-chain is one completed line; aggregate lookup and module retirement
  still require field-by-field analysis and adapters.

## Suggested Next

Execute Step 2: classify the accepted target-neutral return-chain facts apart
from AArch64 target policy and any historical prepared-owned diagnostic or
fallback surfaces. The classification should explicitly account for the idea
180 public-helper contraction so the import note names residual helpers as
absent unless a private/internal implementation detail is still discovered.

## Watchouts

- Keep return-chain separate from Route 1 producer identity, Route 7 comparison
  provenance, predecessor rescans, name matching, and generic route-index
  facades.
- Do not edit implementation, tests, expectations, unsupported markers, closed
  idea files, or draft 155 under this docs/lifecycle-only plan.
- Keep target-local AArch64 ABI/register/home/scratch/emission policy out of
  target-neutral return-chain identity.
- Treat docs that say prepared return-chain helpers "remain public" as
  pre-contraction history unless Step 2 finds a current public helper surface.

## Proof

Docs/lifecycle-only inventory. Read the active plan/source idea, the
return-chain decision doc, closed ideas 176-180, and current readiness docs
that mention return-chain, `PreparedFunctionLookups`, or `PreparedBirModule`.
Ran targeted `rg` scans for those terms under `docs/bir_prealloc_fusion`,
`ideas/open`, and `ideas/closed`. No build/test subset was required and no
`test_after.log` was generated.
