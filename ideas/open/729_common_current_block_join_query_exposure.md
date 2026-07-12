# Common Current-Block Join Query Exposure

Status: Open
Type: common prepared-query contract repair
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
Blocks: `ideas/open/709_aarch64_named_handoff_materializer_cleanup.md`

## Goal

Expose the existing attached current-block join routing authority through a
typed common query contract so target materializers can consume it directly
without building target-local routing arrays or repeating prepared lookup
reasoning.

## Why This Idea Exists

Idea 709's Step 3 retirement search found executable AArch64 current-block join
routing reconstruction in `dispatch.cpp`. The authoritative prepared relation
is already attached and queried by
`query_attached_current_block_join_routing`, but that query is private to
`dispatch_producers.cpp`. The public incoming-expression API accepts a prebuilt
`CurrentBlockJoinPreparedQueryRouting` array, forcing the target owner to
reconstruct executable routing state before it can ask the question.

This is a producer/query boundary gap, not remaining consumer cleanup. It must
be repaired independently before idea 709 can finish its retirement guard.

## In Scope

- Identify the lowest common owner for the attached current-block join
  consumption query and its stable typed inputs.
- Provide direct typed queries for the required incoming-expression and source
  roles without a target-built per-block boolean routing array.
- Preserve owner attachment, freshness, stable value identity, block identity,
  semantic role, ambiguity, and fail-closed behavior from the existing
  prepared authority contract.
- Migrate the bounded AArch64 callers off
  `CurrentBlockJoinPreparedQueryRouting` and delete that array/build surface
  once no executable caller remains.
- Hand the consumer retirement route back to idea 709, including the locally
  replaceable address-materialization lookup noted by its Step 3 audit.
- Add focused positive and negative contract proof plus the affected AArch64
  current-block/join proof.

## Out Of Scope

- Changing how authoritative current-block incoming-expression facts are
  produced or allowing Route 5 diagnostic identity to authorize them.
- Broad prepared lookup, BIR schema, CFG, publication, or target materializer
  redesign.
- x86 or RV64 consumer migration.
- Completing idea 709's unrelated address, memory, call, global, or general
  route-retirement work inside this initiative.
- Expectation weakening, supported-path downgrades, or assembly-only proof.

## Acceptance Criteria

- A target caller can query attached current-block join consumption by typed
  block/value/role authority without constructing a target-local routing
  array or scanning instructions to cache boolean answers.
- Missing owner attachment, stale or incomplete identity, disagreement,
  ambiguity, and unsupported roles remain explicit and fail closed.
- `CurrentBlockJoinPreparedQueryRouting` and its builder have no executable
  AArch64 consumer and are deleted rather than renamed.
- Focused common query positives and fail-closed negatives cover more than one
  instruction shape, and affected AArch64 current-block/join behavior remains
  green without expectation changes.
- Idea 709 can resume its Step 3 retirement search without requiring another
  producer/query contract expansion.

## Reviewer Reject Signals

- The implementation moves or renames the per-block boolean routing array
  while retaining the same target-local reconstruction.
- `dispatch.cpp` copies the private query's prepared lookup, value-id, block,
  or role reasoning instead of consuming common typed authority.
- Route 5 identity, result-name matching, instruction position, or a unique
  named testcase becomes routing authority.
- Missing, stale, incomplete, ambiguous, or mismatched authority is promoted
  to available, or a supported-path expectation is weakened.
- Helper renames, expectation rewrites, status reclassification, or
  classification-only changes are claimed as capability progress.
- The exact target-built routing-array failure mode survives behind a new
  abstraction name.
- The slice broadens into unrelated prepared production, BIR schema, other
  targets, or the rest of idea 709's retirement work.
