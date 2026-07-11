# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Complete the prepared block-entry publication payload

## Just Finished

- Plan review split Step 3 after executor discovery showed that
  `PreparedCurrentBlockEntryPublication` does not yet carry the complete stable
  semantic payload needed to replace the Route 4 common block-entry adapter.
- The prepared result currently owns status, destination IDs/name ID, the
  publication bundle instruction index, and attribution status, but common MIR
  still obtains destination name text/type and raw instruction/PHI/value
  pointers by rebuilding and inspecting a Route 4 index.
- Raw pointer identity must be retired rather than reconstructed or retained
  as prepared/common executable authority.

## Suggested Next

- Execute Step 3.1 only: enrich the prepared producer/result with the complete
  stable successor and destination payload, then run the supervisor-selected
  focused producer proof. Leave common MIR and AArch64 adaptation for Step 3.2.

## Watchouts

- Do not solve the blocker by retaining or reconstructing `bir::Block*`,
  `bir::Inst*`, `bir::PhiInst*`, or `bir::Value*` identity as authority.
- Step 3.1 owns only prepared producer enrichment and focused producer proof;
  common adapter and AArch64 signature/result compatibility belong to Step 3.2.
- Preserve missing, incomplete, ambiguous, unsupported, and mismatched
  fail-closed behavior at the common-MIR boundary.
- Eighteen Route 1 spellings remain in other bounded memory/publication/
  edge-join adapters; they are not same-block producer authority and should
  migrate with their owning families rather than being mechanically renamed.
- Do not expand this repair into Route 5 edge/join work.
- Keep target materializer migration in ideas 708-710 and stack-destination
  authority work in idea 707.

## Proof

- Lifecycle-only plan repair; no code proof was run by the plan owner.
- Step 3.1 requires fresh focused prepared-producer proof selected by the
  supervisor before Step 3.2 begins.
