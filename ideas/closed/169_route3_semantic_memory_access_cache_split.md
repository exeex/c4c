# 169 Route 3 semantic memory/access cache split

## Goal

Migrate one pure semantic memory/source consumer group to Route 3 BIR
memory/access records while leaving target addressing and layout consumers on
their prepared owners.

## Source

This idea comes from Phase C:

- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Route 3 row under `## Step 4 Follow-Up Ideas And Proof Routes`

## Scope

- Direct memory access identity.
- Same-block global-load access.
- Load-local stored-value source facts.
- Memory, call, global, or store-retargeting consumers that only need semantic
  source/access identity.

## Out Of Scope

- `PreparedAddress` or `PreparedMemoryAccess` wholesale migration.
- Frame slot ids, byte offsets, size/align layout, relocation/TLS details,
  base-plus-offset legality, AArch64 memory operand formation, and target
  addressing API parameters.

## Acceptance Criteria

- The selected semantic consumer reads `Route3MemoryAccessIndex`.
- Prepared answers remain oracle-visible during migration.
- Target addressing/layout facts remain prepared/prealloc or target-owned.
- Cache/API contraction only affects semantic identity surfaces no longer
  publicly consumed.

## Completion Note

Closed after migrating the selected AArch64 FP same-block global-load value
materialization family to Route 3 semantic same-block global-load identity.
Prepared `PreparedMemoryAccess` remains the target-addressing and
address-policy source for that consumer.

No public prepared same-block global-load helper contraction was made because
direct non-FP consumers remain in
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` and
`src/backend/mir/aarch64/codegen/globals.cpp`. That residual work is outside
this closed idea's one-selected-consumer acceptance scope and should be handled
by a future bounded Route 3 candidate if continued.

Close proof used the existing matching narrow logs for:
`backend_aarch64_prepared_memory_operand_records` and
`backend_prepared_lookup_helper`.

## Proof Route

Prove BIR/prepared equivalence for semantic access/source answers, then run
targeted AArch64 memory/global/store-retargeting subsets. Escalate if public
memory/addressing headers or context projections change.

## Reviewer Reject Signals

- Copying prepared address/layout payloads into BIR.
- Replacing indexed access with broad scans.
- Claiming memory progress through expectation rewrites or named-case matching.
