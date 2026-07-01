Status: Active
Source Idea Path: ideas/open/498_dynamic_local_array_ordered_effect_source_stream_builder.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Prepared/BIR Effect-Source Surfaces

# Current Packet

## Just Finished

Step 1 audit completed for the prepared/BIR effect-source surfaces required by
idea 498. Exact builder input surfaces are recorded in
`build/agent_state/498_step1_effect_source_surface_audit/summary.md`.

Audit result: production has comparable source data for the required families,
but no production builder currently merges them into one ordered local-array
bounded effect-source stream. The precise lower blocker is the missing stream
builder/writer, not missing individual source surfaces.

Builder inputs found:

- Interval boundaries: `bir::LocalArraySelectedProofEdgePathRecord` for the
  proof-source coordinates and selected path, plus
  `bir::LocalArrayEndpointBridgeRecord` for prepared block index, BIR block
  label, endpoint instruction index, source object, derivation result, and
  dynamic-index match.
- Assignments/redefinitions: ordered traversal of
  `bir::Function::blocks[].insts` using result-defining `BinaryInst`,
  `SelectInst`, `CastInst`, `PhiInst`, `CallInst` result/result lanes,
  `LoadLocalInst`, and `LoadGlobalInst`.
- Memory accesses: `prepare::PreparedAddressingFunction::accesses` through
  `PreparedMemoryAccess` and `PreparedMemoryAccessLookups`, keyed by
  function/block label/`inst_index`, with result/stored value names,
  volatility, address space, and prepared address identity.
- Phi/alias transfers: BIR `PhiInst`, `PreparedControlFlowFunction`
  `join_transfers`, `PreparedSelectCarrierAliasAuthorityRecords`, and
  `PreparedEdgePublication` source-producer coordinates.
- Calls/helpers: BIR `CallInst`; `PreparedCallPlansFunction::calls`;
  `PreparedCallPlan` block/instruction coordinates, clobbered registers,
  preserved values, and `plan_prepared_call_boundary_effects`; runtime-helper
  clobber/resource ownership surfaces for helper calls.
- Inline asm: BIR `CallInst::inline_asm` plus `PreparedInlineAsmCarriers` /
  `PreparedInlineAsmCarrier` with block/index, side effects, operands,
  clobbers, result home, and missing/unsupported facts.
- Publications: `PreparedEdgePublicationLookups` / `PreparedEdgePublication`
  and `PreparedStoreSourcePublicationPlans`, including source producer
  coordinates, memory-source details, move bundle pointer, parallel-copy facts,
  and source/destination homes.
- Move bundles: `PreparedValueLocationFunction::move_bundles`,
  `PreparedMoveBundleLookups`, and `PreparedMoveResolution`, keyed by
  phase/block/instruction with from/to values, destination storage, ABI index,
  authority, and out-of-SSA labels.
- Parallel copies: `PreparedControlFlowFunction::parallel_copy_bundles` and
  `PreparedParallelCopyBundle`, with predecessor/successor labels, execution
  site/block, moves, steps, and cycle state; publications link back through
  `PreparedEdgePublication::parallel_copy_*`.
- Unknown modeled effects: default unknown BIR/effect categories, volatile or
  incomplete prepared memory accesses, unknown call/helper resource policy,
  inline-asm unsupported/missing facts, unavailable publication statuses,
  unsupported move operations, and unsupported parallel-copy execution sites.

## Suggested Next

Execute Step 2 by defining the ordered effect-source contract over the audited
inputs: coordinate type, interval inclusion/exclusion, family-to-status mapping,
stream ordering/tie-breaks, and fail-closed behavior for missing coordinates or
unsupported modeled effects.

## Watchouts

- Do not accept `LocalArrayIntervalEffectInputs` booleans as builder evidence;
  they remain classifier inputs after a real stream exists.
- Do not treat `lir_producer_instruction_index` as a prepared traversal index,
  BIR instruction index, or ordered effect endpoint.
- Same-block proof-source ordering is still a contract issue for Step 2; the
  audited endpoint side has prepared block index and endpoint instruction
  index, but proof-source-to-endpoint ordering must be defined explicitly.
- Idea 497 remains parked and idea 494 remains fail-closed until this builder
  can populate ordered effect-source records for the selected interval.

## Proof

```sh
git diff --check
```

Result: passed with no output. Output was preserved in `test_after.log`.
