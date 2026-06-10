# 154 Phase D MIR consumer BIR view switch plan

## Goal

Plan how MIR/codegen consumers should switch from reading `Prepared*` wrappers
to reading BIR nodes and BIR-owned annotations.

This is Phase D of the BIR/prealloc thinning program. It is analysis-only and
must produce follow-up ideas for consumer migration, not directly convert
AArch64, x86, or riscv lowering.

## Direction

The final consumer contract should be BIR annotated view -> MIR lowering. The
MIR layer should not need a separate `PreparedBirModule` container when BIR
node identity plus annotations are sufficient.

## Questions

- Which MIR/AArch64 consumers currently require `PreparedBirModule`,
  `PreparedFunctionLookups`, or domain `Prepared*` query structs?
- Which consumers can switch first to BIR annotations with minimal blast radius?
- Which consumers need adapter APIs during migration?
- Which consumers reveal missing Phase A/B normalization or annotation work?
- What proof ladder keeps backend behavior stable across the switch?

## Required Analysis

Map consumer families:

- AArch64 traversal and dispatch
- calls
- memory and addressing
- value materialization and publication
- comparison and ALU
- edge copies and control flow
- wide values and runtime helpers
- future x86/riscv planned consumers, at least at interface level

## Expected Output

The closure note must contain:

- a consumer dependency map from prepared APIs to proposed BIR view APIs;
- an ordered migration ladder;
- adapter boundaries that may be needed temporarily;
- follow-up ideas for each safe consumer migration slice;
- explicit blockers that must be solved by Phase A/B/C before conversion.

## Reject Signals

- Switching MIR consumers before BIR annotations are available.
- Reintroducing local BIR scans, predecessor rescans, or name matching in MIR.
- Mixing consumer migration with unrelated target emission rewrites.
- Treating AArch64-only convenience as the final cross-arch BIR view.
