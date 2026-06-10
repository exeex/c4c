# 152 Phase B BIR annotation schema candidate audit

## Goal

Audit which `Prepared*` fields should become BIR annotation schema after Phase A
identifies the BIR-owned semantic relationships.

This is Phase B of the BIR/prealloc thinning program. It is analysis-only and
must produce follow-up ideas for annotation schema work rather than directly
changing the schema.

## Direction

The desired final shape is that MIR/codegen can consume BIR nodes plus typed
annotations, instead of consuming a parallel `PreparedBirModule` universe.
Prealloc may still fill annotations, but the container and identity should be
BIR-owned.

## Questions

- Which `Prepared*` fields should be copied or fused into BIR node, block,
  edge, function, or module annotations?
- Which fields are derived caches that should never become durable schema?
- Which fields are layout/ABI decisions that should be annotations produced by
  a thin prealloc pass?
- Which fields are target-facing conveniences and should remain outside BIR?
- What annotation identity model avoids duplicating value ids, names, block
  labels, and instruction indexes?

## Required Analysis

Classify candidate annotation locations:

- BIR value annotations
- BIR instruction annotations
- BIR terminator annotations
- BIR block annotations
- BIR edge annotations
- BIR function annotations
- BIR module-level lowering metadata

Inspect at least:

- value-home and storage facts
- call argument/result/outgoing-stack facts
- publication and edge-publication facts
- move-bundle and parallel-copy facts
- address/materialization facts
- comparison and materialized-condition facts
- return-chain facts

## Expected Output

The closure note must contain:

- a candidate BIR annotation schema map;
- a list of `Prepared*` fields that should stay private/cache-only;
- a list of fields that are target-facing and must not enter BIR;
- follow-up ideas for schema prototypes by domain;
- migration constraints for keeping AArch64 behavior stable while annotations
  are introduced.

## Reject Signals

- Copying whole `Prepared*` structs into BIR without field classification.
- Creating annotations that expose AArch64 register names, scratch policy, or
  instruction spelling.
- Losing cheap lookup identity or forcing consumers into expensive scans.
- Ignoring future x86/riscv consumers when defining schema shape.
