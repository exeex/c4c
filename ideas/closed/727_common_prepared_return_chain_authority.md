# Common Prepared Return-Chain Authority

Status: Open
Type: common prepared-MIR producer/query contract repair
Discovered by: `ideas/open/709_aarch64_named_handoff_materializer_cleanup.md`
Unblocks: `ideas/open/709_aarch64_named_handoff_materializer_cleanup.md`

## Completion Note (2026-07-12)

Closed after the traversal-attached `PreparedObjectReturnChainClassification`
proved the full consumer handoff required by idea 709. The available relation
contains the authenticated start and terminal homes, terminal return-ABI move,
binding and register placement, the first successor ALU non-chain operand home,
and each intervening move, scalar producer, operand role, and freshness
authority. Missing or incoherent evidence remains typed and fail closed.

The focused common contract covers two distinct valid chain shapes and the
nearby negative matrix. A matching close-time regression comparison passed.
Idea 709 may now resume at Step 2.1 and delete the AArch64 reconstruction and
its generated-lookup fallback without requiring new common authority.

## Goal

Publish a traversal-attached typed common relation for prepared scalar return
chains so target materializers can consume the terminal return-ABI value home,
the first successor ALU non-chain operand home, and their authenticated
cross-instruction relation without rediscovering producer semantics.

## Why This Exists

The existing `PreparedObjectTraversalEvent` and
`PreparedObjectMoveBundleConsumerClassification` describe one traversal site,
its move bundle, per-source freshness, and optional stack fan-in authority.
They do not express the multi-event relation currently reconstructed by
`find_prepared_return_chain_facts` in AArch64 `alu.cpp`: successive value
moves, same-block scalar producers, chain-operand identity, the terminal ABI
return home, and the first non-chain ALU operand home. That missing authority
belongs at the common prepared producer/query boundary.

## In Scope

- Inventory the semantic inputs and invariants of the existing AArch64
  reconstruction and identify the earliest common prepared owner for each.
- Define a typed relation and precise available, absent, stale, ambiguous,
  inconsistent, and unsupported outcomes.
- Produce the relation from common prepared facts and attach it to the
  appropriate traversal consumer view without target assistance.
- Preserve value, instruction, move, producer, operand-role, home, ABI-return,
  and freshness identity across the relation.
- Add focused common producer/query proof for valid multi-instruction chains
  and nearby fail-closed cases.
- Hand the proven typed query back to idea 709 for AArch64 consumption and
  removal of `find_prepared_return_chain_facts`.

## Out Of Scope

- AArch64 instruction selection, ABI realization, or consumer migration.
- X86 or RV64 materializer changes.
- General scalar optimization, scheduling, register allocation, or arbitrary
  dataflow analysis beyond the prepared return-chain contract.
- Target-local fallback, route/index recreation, or reconstruction from raw
  instruction shape when common authority is unavailable.
- Expectation weakening or fixture-only injection presented as production.

## Acceptance Criteria

- The typed common view identifies the terminal return-ABI home and first
  successor ALU non-chain operand home as one authenticated relation.
- Production owns the cross-instruction move/scalar-producer chain and rejects
  missing, stale, ambiguous, inconsistent, unsupported, or non-adjacent
  evidence precisely and fail closed.
- Focused common tests cover more than one valid chain shape and nearby
  negative states without target-side synthesis or expectation changes.
- Idea 709 can consume only traversal-attached authority and delete its local
  semantic reconstruction and lookup fallback.

## Reviewer Reject Signals

- Matching AArch64 test names, opcode sequences, fixed instruction positions,
  register names, or one known return chain instead of defining a general
  typed relation.
- Injecting the positive relation only in a fixture or synthesizing it inside
  AArch64, x86, or RV64 codegen.
- Downgrading supported return behavior, weakening expectations, or accepting
  missing/stale/ambiguous evidence as available.
- Helper renames, status relabeling, diagnostic-only changes, or moving
  `find_prepared_return_chain_facts` unchanged into a nominally common file
  claimed as capability progress.
- Broad scalar, regalloc, scheduling, ABI, or target-materializer rewrites
  outside the common prepared producer/query seam.
- Retaining the exact cross-event reconstruction failure behind a new
  abstraction name or requiring a target consumer to complete the relation.
