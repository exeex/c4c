# Prepared Local Address Base-Plus-Offset Boundary Evidence

Status: Open
Type: Focused producer/consumer evidence gap
Parent: `ideas/closed/547_bir_local_memory_call_metadata_boundary_review.md`
Owning Layer: Prepared contract and RV64 prepared local-memory consumer

## Goal

Prove whether retained `prepared_local_address_base_plus_offset_missing`
`unsupported_local_memory_access` rows are prepared-owned or RV64-owned before
any local-memory lowering repair proceeds.

## Why This Exists

The boundary review retained 41 local-memory rows from per-case RV64 torture
logs. Representative cases such as `src/20000519-1.c`, `src/20070212-3.c`,
`src/pr35800.c`, and `src/pr65369.c` reached the RV64 object route and failed
because RV64 required a prepared frame-slot or pointer-value
base-plus-offset local-memory address.

Step 3 could not honestly assign first ownership because the retained logs do
not include focused prepared dumps that prove whether the prepared
base-plus-offset fact is absent/incoherent or present and rejected by RV64.

## In Scope

- Rerun one retained representative, preferably `src/20000519-1.c`, through
  semantic BIR, prepared BIR, and RV64 object routes in the same workspace
  snapshot.
- Inspect BIR `load_local`/`store_local` address records and prepared
  `memory_access` / `address_materialization` rows.
- Prove `PreparedAddressBaseKind`, `can_use_base_plus_offset`, size,
  alignment, default-space/non-volatile status, pointer-value register homes,
  and immediate encodability for the failing access.
- Classify the first owner:
  - prepared-owned if the required prepared base-plus-offset fact is missing
    or incoherent
  - RV64-owned if the required prepared fact is present and
    `fragment_for_prepared_store_local` /
    `fragment_for_prepared_load_local` still rejects it
- Create or execute a repair only after the first bad fact is named.

## Out Of Scope

- RV64 target-shaped recovery from raw LIR/BIR address expressions.
- Combining this route with direct-call metadata repair.
- Reclassifying latest summary-file rows without a fresh matching scan.
- Weakening unsupported accounting, tests, or prepared admission contracts.

## Acceptance Criteria

- A focused proof log names the representative case, exact failing access, and
  first bad prepared/RV64 fact.
- The owner classification is tied to visible semantic BIR, prepared BIR, and
  RV64 evidence from the same run.
- If a repair is made, it adds focused BIR/prepared coverage first and then
  proves representative RV64 behavior without expectation downgrades.
- The old `unsupported_local_memory_access` failure mode is either removed by
  the named capability repair or explicitly remains blocked with the missing
  evidence recorded.

## Reviewer Reject Signals

- Reject any RV64 lowering that reconstructs frame-slot, pointer-value, or
  offset facts from target-specific instruction shapes instead of consuming
  prepared facts.
- Reject claiming prepared ownership without a focused prepared dump showing a
  missing or incoherent `PreparedMemoryAccess` / address-materialization fact.
- Reject claiming RV64 ownership without proof that the required prepared
  base-plus-offset fact is present and still rejected by the RV64 consumer.
- Reject testcase-shaped shortcuts for `src/20000519-1.c`,
  `src/20070212-3.c`, `src/pr35800.c`, `src/pr65369.c`, or any other named
  torture case.
- Reject expectation rewrites, unsupported downgrades, diagnostic renames, or
  helper-only refactors as local-memory capability progress.
- Reject broad local-memory rewrites that do not name the first bad fact at
  the prepared/RV64 boundary.
