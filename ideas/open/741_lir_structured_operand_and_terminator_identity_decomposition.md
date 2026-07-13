# Structured LIR Operand And Terminator Identity

Status: Open (active decomposition)
Type: typed LIR identity-carrier decomposition
Blocked Consumer: ideas/open/734_lir_to_new_bir_container_completeness.md

## Goal

Give producer-emitted LIR instruction operands and terminator values the
smallest generic typed identity carriers needed for lossless, verifiable use by
the blocked new-BIR receiver initiative.

## Why This Exists

Idea 734 reached the first function-body boundary after completing typed
module/global and direct scalar signature receipt. Actual producers show that
`LirLoadOp`, `LirStoreOp`, `LirGepOp`, and many other instructions use
`LirOperand`, whose value or global identity is only classified text, while
non-void `LirRet` carries only optional `value_str` plus `type_str`.

The focused cases `global_store.c`, `defined_pointer_global_pointer.c`,
`defined_global_array.c`, and `riscv64_zero_aggregate_global_storage.c` now
clear module/global/signature receipt and stop at those identity boundaries.
Continuing inside 734 would require forbidden text-derived reconstruction or an
out-of-scope LIR change. This separate initiative decomposes and owns the
producer-side identity gap before handing stable typed authority back to 734.

## In Scope

- inventory every `LirInst` and `LirTerminator` operand, result, immediate, and
  symbol field by its actual identity authority: `LirValueId`, `LinkNameId`,
  typed immediate, structured-but-text-only `LirOperand`, or raw text
- establish a checked authority matrix naming the producer, carrier, verifier,
  focused probe, and disposition for every row
- confirm or extract focused one-contract probes under `tests/backend/case/`
  for global-symbol load/store identity, global aggregate/array address identity,
  and non-void return-value identity
- contrast already-stable seams, especially legacy `LirValueId` rows and
  direct-call `LinkNameId`, so no duplicate or parallel value model is added
- bind each focused probe to one generic carrier/producer/verifier contract
  before editing LIR schema or producers
- implement only the smallest evidence-proven typed LIR carrier, producer, and
  verifier changes that make the owned rows lossless and structurally checked
- preserve existing display text only as presentation or parity evidence after
  structured authority exists
- provide a checked handoff matrix and focused proof that lets idea 734 resume
  new-BIR instruction and terminator receipt

## Focused Seams And Probes

1. Global-symbol load/store identity: use `global_store.c` and
   `defined_pointer_global_pointer.c`; extract a smaller case only if either
   file conflates contracts.
2. Global aggregate/array address identity for `LirGepOp`: use
   `defined_global_array.c`.
3. Non-void return-value identity: use
   `riscv64_zero_aggregate_global_storage.c` as integration evidence and
   extract a minimal scalar-return probe if needed to isolate the carrier.
4. Stable-identity contrast: retain and document existing `LirValueId` and
   direct-call `LinkNameId` paths as the minimality boundary.

## Progress Contract

Progress first requires a checked authority matrix and focused probes. No LIR
schema edit is justified until each probe is bound to one generic typed
carrier, producer, and verifier contract. Implementation progress then means
the smallest such carriers are populated by ordinary producers, rejected when
malformed, and proven across neighboring rows without text parsing.

## Out Of Scope

- new-BIR instruction, terminator, container, builder, verifier, or importer
  implementation; those remain owned by idea 734
- parsing or matching rendered operand text, symbol names, `value_str`,
  `type_str`, printer output, or testcase identity to recover semantics
- testcase-specific carrier fields, named-case dispatch, allowlists, or
  expectation downgrades
- a parallel LIR value system, broad LIR redesign, or replacement of stable
  `LirValueId` / `LinkNameId` paths that already carry sufficient authority
- downstream canonicalization, target lowering, ABI placement, allocation,
  MIR, emission, or assembler work
- closing or superseding idea 734

## Acceptance Criteria

- A checked matrix enumerates every current `LirInst` and `LirTerminator`
  operand/result/symbol field with its exact identity authority and no catch-all
  row.
- Each focused seam has a one-contract probe and is bound to one generic typed
  carrier, producer, and verifier obligation before implementation begins.
- Global-symbol load/store, aggregate/array address, and non-void return values
  preserve stable typed identity without rendered-text reconstruction.
- Existing stable `LirValueId` and direct-call `LinkNameId` seams remain the
  ordinary model; no duplicate value or symbol authority is introduced.
- Malformed, missing, conflicting, or cross-owner identities reject through
  reachable LIR verification with focused neighboring negative coverage.
- A fresh build, focused proof, backend regression checkpoint, and final
  authority-matrix audit pass before handoff to idea 734.
- The handoff names the exact 734 importer rows unblocked and leaves all
  new-BIR receipt implementation to 734.

## Reviewer Reject Signals

- Reject schema edits made before a checked authority row and focused probe
  bind the change to a generic carrier/producer/verifier contract.
- Reject parsing, matching, or normalizing `LirOperand` text, `value_str`,
  `type_str`, symbol spelling, printer output, or testcase names to invent
  identity.
- Reject a carrier that only fixes one named case, adds an allowlist, weakens an
  unsupported expectation, or leaves neighboring producer rows text-only.
- Reject a parallel value/symbol model or broad redesign when existing
  `LirValueId`, `LinkNameId`, or typed-immediate conventions can be extended
  minimally.
- Reject new-BIR importer/container work, downstream lowering, target
  interpretation, canonicalization, allocation, MIR, or emission in this idea.
- Reject claiming completion from classification-only documentation, renamed
  helpers, or green tests that do not prove producer population and verifier
  rejection of malformed identity state.
- Reject closing or superseding idea 734; this initiative must hand the typed
  carrier capability back to that open consumer.
