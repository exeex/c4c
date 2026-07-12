# Prepared Return-Chain Shape Authority Decomposition

Status: Open
Type: common prepared-MIR production-shape decomposition
Discovered by: `ideas/open/709_aarch64_named_handoff_materializer_cleanup.md`
Blocks: `ideas/open/709_aarch64_named_handoff_materializer_cleanup.md` Step 2.1

## Goal

Decompose the production prepared return-chain authority gap into focused,
independently provable common producer seams before attempting another AArch64
consumer deletion.

## Why This Exists

Two Step 2.1 attempts have collided with the same production-authority claim.
The first real consumer attempt found `Stale` authority after synthetic common
classification passed. After common publication attribution was repaired, the
next attempt moved the first bad fact to the public multi-link external add/sub
path: its first ALU traversal event classifies `StructurallyIncomplete`, while
synthetic one-/two-link fixtures and a representative one-link AArch64 builder
classify `Available`. The focused guard correspondingly introduces
`backend_cli_aarch64_asm_external_return_add_sub_chain_smoke` as a new failure.

This moving boundary shows that another consumer-only retry or monolithic
external-test patch would overfit the symptom. The production family must be
split into capability probes and each failure bound to its earliest common
owner.

## Seams To Split

1. Normal one-link public prepared-chain publication.
2. Normal multi-link successor bundle attribution and adjacency.
3. Terminal move-bundle publication and unique `FunctionReturnAbi` binding.
4. Traversal attachment/classification parity between focused builders and
   public external inputs.

## Probe Contract

- Prefer one capability per focused case under `tests/backend/case/` for the
  one-link and multi-link public return-chain shapes.
- Add focused assertions to existing common or AArch64 contract binaries only
  when direct internal status observation is required.
- Retain the existing external add/sub-chain smoke as integration proof, not as
  the implementation target.
- Record each probe's expected classification and the earliest common producer
  seam that owns any divergence.

## In Scope

- Establish the focused failing-family baseline without changing expectations.
- Extract one-link and multi-link production-shape probes.
- Isolate successor attribution/adjacency, terminal binding, and traversal
  attachment as separate common authority contracts.
- Repair only the narrowest generic common producer seam proved deficient.
- Re-run focused cases, direct classification proof, the external integration
  case, and a matching supervisor-selected regression guard.

## Out Of Scope

- AArch64 consumer deletion or target-side authority reconstruction.
- Reopening or expanding idea 709 while this decomposition is active.
- Reopening idea 727; its completed publication-attribution work remains
  historical input, not the active contract.
- Test expectation weakening, named-case matching, or fixed opcode/position
  logic.
- Broad prepared-MIR, ABI, regalloc, or traversal redesign beyond a probe-owned
  common producer seam.

## Acceptance Criteria

- Focused one-link and multi-link production probes expose one primary contract
  each and preserve the external add/sub-chain case as integration proof.
- Every observed `Stale` or `StructurallyIncomplete` result is attributed to
  one earliest common producer seam with direct internal evidence.
- At least the narrowest deficient seam is repaired generically, with public
  and focused classification parity and no AArch64 synthesis.
- Matching proof shows the owned failure family shrinks without expectation
  changes or new nearby failures.
- Lifecycle review can state precisely whether idea 709 Step 2.1 may resume or
  which remaining decomposition seam still blocks it.

## Reviewer Reject Signals

- Matching the external add/sub-chain testcase, its opcode sequence, literal
  values, fixed instruction positions, or register names.
- Creating smaller cases that remain copies of the monolithic integration test
  instead of isolating one publication, adjacency, binding, or attachment seam.
- Weakening the external smoke or focused expectations, relabeling
  `StructurallyIncomplete`, or accepting unavailable authority as usable.
- Claiming helper renames, classification-only changes, or fixture injection as
  production capability progress.
- Reconstructing the missing relation in AArch64 or retaining the same failure
  behind a new target-local abstraction.
- Broad common/backend rewrites before a focused probe identifies their owned
  seam.
